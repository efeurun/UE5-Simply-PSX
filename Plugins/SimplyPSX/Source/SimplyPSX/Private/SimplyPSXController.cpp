// UE5 Simply-PSX

#include "SimplyPSXController.h"

#include "Components/PostProcessComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogSimplyPSX, Log, All);

ASimplyPSXController::ASimplyPSXController()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PSXPostProcess"));
	SetRootComponent(PostProcess);
	PostProcess->bUnbound = true;
	PostProcess->bEnabled = false;
	PostProcess->BlendWeight = 0.0f;
	PostProcess->Priority = 100.0f;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ScreenAsset(
		TEXT("/SimplyPSX/Materials/M_PSXScreen.M_PSXScreen"));
	ScreenMaterial = ScreenAsset.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> CollectionAsset(
		TEXT("/SimplyPSX/Materials/MPC_PSX.MPC_PSX"));
	GeometryParameters = CollectionAsset.Object;
}

void ASimplyPSXController::ApplySettings()
{
	if (!IsTemplate() && bRegisteredForEffects)
	{
		RefreshWorldControllers(GetWorld());
	}
}

void ASimplyPSXController::SetEnabled(bool bNewEnabled)
{
	bEnabled = bNewEnabled;
	ApplySettings();
}

void ASimplyPSXController::ApplyPreset(ESimplyPSXPreset NewPreset)
{
#if WITH_EDITOR
	if (GetWorld() && !GetWorld()->IsGameWorld() && !IsTemplate())
	{
		Modify();
	}
#endif
	Preset = NewPreset;
	if (NewPreset != ESimplyPSXPreset::Custom)
	{
		bEnabled = true;
		bPixelationEnabled = true;
		bWobbleEnabled = true;
		bTextureWarpEnabled = true;
		ColorBits = 5;

		switch (NewPreset)
		{
		case ESimplyPSXPreset::Subtle540:
			PixelHeight = WobbleGridHeight = 540;
			WobbleStrength = TextureWarpStrength = 0.15f;
			bColorReductionEnabled = bDitheringEnabled = false;
			DitherStrength = 0.25f;
			break;
		case ESimplyPSXPreset::Balanced360:
			PixelHeight = WobbleGridHeight = 360;
			WobbleStrength = TextureWarpStrength = 0.4f;
			bColorReductionEnabled = bDitheringEnabled = true;
			DitherStrength = 0.5f;
			break;
		case ESimplyPSXPreset::Chunky240:
			PixelHeight = WobbleGridHeight = 240;
			WobbleStrength = TextureWarpStrength = 1.0f;
			bColorReductionEnabled = bDitheringEnabled = true;
			DitherStrength = 1.0f;
			break;
		default:
			break;
		}
	}
	ApplySettings();
}

void ASimplyPSXController::ApplySelectedPreset()
{
	ApplyPreset(Preset);
}

void ASimplyPSXController::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplySettings();
}

void ASimplyPSXController::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
	bRegisteredForEffects = !IsTemplate();
	ApplySettings();
}

void ASimplyPSXController::PostUnregisterAllComponents()
{
	UnregisterEffects();
	Super::PostUnregisterAllComponents();
}

void ASimplyPSXController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterEffects();
	Super::EndPlay(EndPlayReason);
}

void ASimplyPSXController::Destroyed()
{
	UnregisterEffects();
	Super::Destroyed();
}

void ASimplyPSXController::UnregisterEffects()
{
	bRegisteredForEffects = false;
	ReleaseEffects();
	RefreshWorldControllers(GetWorld());
}

void ASimplyPSXController::ReleaseEffects()
{
	bIsActiveController = false;
	if (PostProcess)
	{
		PostProcess->bEnabled = false;
		PostProcess->BlendWeight = 0.0f;
	}

	// Only a controller that previously published this collection may clear it.
	// RefreshWorldControllers clears old owners before the new owner publishes.
	if (PublishedGeometryParameters && GetWorld())
	{
		if (UMaterialParameterCollectionInstance* Collection =
			GetWorld()->GetParameterCollectionInstance(PublishedGeometryParameters))
		{
			Collection->SetScalarParameterValue(TEXT("PSX_Enabled"), 0.0f);
			Collection->SetScalarParameterValue(TEXT("PSX_WobbleStrength"), 0.0f);
			Collection->SetScalarParameterValue(TEXT("PSX_TextureWarpStrength"), 0.0f);
		}
	}
	PublishedGeometryParameters = nullptr;
}

void ASimplyPSXController::RefreshWorldControllers(UWorld* World)
{
	if (!World || World->WorldType == EWorldType::EditorPreview || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	ASimplyPSXController* Owner = nullptr;
	TArray<ASimplyPSXController*> Controllers;
	for (TActorIterator<ASimplyPSXController> It(World); It; ++It)
	{
		ASimplyPSXController* Controller = *It;
		if (!Controller->bRegisteredForEffects || Controller->IsTemplate() || Controller->IsActorBeingDestroyed())
		{
			continue;
		}
		Controllers.Add(Controller);
		Controller->ReleaseEffects();
		if (!Owner || Controller->GetPathName().Compare(Owner->GetPathName(), ESearchCase::CaseSensitive) < 0)
		{
			Owner = Controller;
		}
	}

	for (ASimplyPSXController* Controller : Controllers)
	{
		if (Controller != Owner && !Controller->bWarnedDuplicate)
		{
			UE_LOG(LogSimplyPSX, Warning, TEXT("PSX: %s is suppressed; %s owns this world's PSX settings. Keep one controller per world."),
				*Controller->GetPathName(), *Owner->GetPathName());
			Controller->bWarnedDuplicate = true;
		}
	}

	if (Owner)
	{
		Owner->bIsActiveController = true;
		Owner->bWarnedDuplicate = false;
		Owner->PublishSettings();
	}
}

void ASimplyPSXController::PublishSettings()
{
	if (!PostProcess || !GetWorld())
	{
		return;
	}

	// The component is owned exclusively by this controller. Replacing its blendable
	// also removes stale transient instances after editor reconstruction or loading.
	PostProcess->Settings.WeightedBlendables.Array.Reset();
	if (ScreenMaterial)
	{
		bWarnedMissingScreenMaterial = false;
		if (!ScreenInstance || ScreenInstanceSource != ScreenMaterial)
		{
			ScreenInstance = UMaterialInstanceDynamic::Create(ScreenMaterial, this);
			ScreenInstanceSource = ScreenMaterial;
			ScreenInstance->SetFlags(RF_Transient);
		}
		ScreenInstance->SetScalarParameterValue(TEXT("PixelHeight"), FMath::Clamp(PixelHeight, 64, 4320));
		ScreenInstance->SetScalarParameterValue(TEXT("PixelationEnabled"), bPixelationEnabled ? 1.0f : 0.0f);
		ScreenInstance->SetScalarParameterValue(TEXT("ColorReductionEnabled"), bColorReductionEnabled ? 1.0f : 0.0f);
		ScreenInstance->SetScalarParameterValue(TEXT("ColorBits"), FMath::Clamp(ColorBits, 2, 8));
		ScreenInstance->SetScalarParameterValue(TEXT("DitheringEnabled"), bDitheringEnabled ? 1.0f : 0.0f);
		ScreenInstance->SetScalarParameterValue(TEXT("DitherStrength"), FMath::Clamp(DitherStrength, 0.0f, 1.0f));
		PostProcess->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, ScreenInstance));
		const bool bAnyScreenEffect = bPixelationEnabled || bColorReductionEnabled || bDitheringEnabled;
		PostProcess->bEnabled = bEnabled && bAnyScreenEffect;
		PostProcess->BlendWeight = PostProcess->bEnabled ? 1.0f : 0.0f;
	}
	else if (bEnabled && !bWarnedMissingScreenMaterial)
	{
		UE_LOG(LogSimplyPSX, Warning, TEXT("PSX: %s has no Screen Material. Assign M_PSXScreen under PSX > Assets."), *GetPathName());
		bWarnedMissingScreenMaterial = true;
	}

	if (GeometryParameters)
	{
		bWarnedMissingCollection = false;
		if (UMaterialParameterCollectionInstance* Collection = GetWorld()->GetParameterCollectionInstance(GeometryParameters))
		{
			Collection->SetScalarParameterValue(TEXT("PSX_Enabled"), bEnabled ? 1.0f : 0.0f);
			Collection->SetScalarParameterValue(TEXT("PSX_WobbleStrength"),
				bEnabled && bWobbleEnabled ? FMath::Clamp(WobbleStrength, 0.0f, 1.0f) : 0.0f);
			Collection->SetScalarParameterValue(TEXT("PSX_WobbleGridHeight"), FMath::Clamp(WobbleGridHeight, 64, 4320));
			Collection->SetScalarParameterValue(TEXT("PSX_TextureWarpStrength"),
				bEnabled && bTextureWarpEnabled ? FMath::Clamp(TextureWarpStrength, 0.0f, 1.0f) : 0.0f);
			PublishedGeometryParameters = GeometryParameters;
		}
	}
	else if (bEnabled && (bWobbleEnabled || bTextureWarpEnabled) && !bWarnedMissingCollection)
	{
		UE_LOG(LogSimplyPSX, Warning, TEXT("PSX: %s has no Geometry Parameters collection. Assign MPC_PSX under PSX > Assets."), *GetPathName());
		bWarnedMissingCollection = true;
	}
}

#if WITH_EDITOR
void ASimplyPSXController::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// Actor labels, folders, transforms and component bookkeeping do not change
	// the selected visual preset. Only edits to actual effect settings make it custom.
	static const TSet<FName> EffectProperties =
	{
		GET_MEMBER_NAME_CHECKED(ASimplyPSXController, bPixelationEnabled),
		GET_MEMBER_NAME_CHECKED(ASimplyPSXController, PixelHeight),
		GET_MEMBER_NAME_CHECKED(ASimplyPSXController, bWobbleEnabled),
		GET_MEMBER_NAME_CHECKED(ASimplyPSXController, WobbleStrength),
		GET_MEMBER_NAME_CHECKED(ASimplyPSXController, WobbleGridHeight),
		GET_MEMBER_NAME_CHECKED(ASimplyPSXController, bTextureWarpEnabled),
		GET_MEMBER_NAME_CHECKED(ASimplyPSXController, TextureWarpStrength),
		GET_MEMBER_NAME_CHECKED(ASimplyPSXController, bColorReductionEnabled),
		GET_MEMBER_NAME_CHECKED(ASimplyPSXController, ColorBits),
		GET_MEMBER_NAME_CHECKED(ASimplyPSXController, bDitheringEnabled),
		GET_MEMBER_NAME_CHECKED(ASimplyPSXController, DitherStrength)
	};
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ASimplyPSXController, Preset))
	{
		ApplyPreset(Preset);
	}
	else if (EffectProperties.Contains(PropertyName))
	{
		Preset = ESimplyPSXPreset::Custom;
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplySettings();
}

void ASimplyPSXController::PostEditUndo()
{
	Super::PostEditUndo();
	ApplySettings();
}
#endif

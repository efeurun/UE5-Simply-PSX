// UE5 Simply-PSX

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SimplyPSXController.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMaterialParameterCollection;
class UPostProcessComponent;

UENUM(BlueprintType)
enum class ESimplyPSXPreset : uint8
{
	Subtle540 UMETA(DisplayName = "Subtle (540p)"),
	Balanced360 UMETA(DisplayName = "Balanced (360p)"),
	Chunky240 UMETA(DisplayName = "Chunky (240p)"),
	Custom UMETA(DisplayName = "Custom")
};

/**
 * Place one in a level to control the PSX screen and compatible surface materials.
 * Settings preview in the editor. At runtime call Apply Settings after changing properties.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Simply PSX Controller"), HideCategories = (Actor, Collision, Input, Replication, Physics, HLOD, Networking))
class SIMPLYPSX_API ASimplyPSXController : public AActor
{
	GENERATED_BODY()

public:
	ASimplyPSXController();

	/** Master switch for every PSX effect, including material wobble and texture warping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PSX|General", meta = (DisplayName = "Enable PSX"))
	bool bEnabled = true;

	/** Selecting a preset in the editor applies it once; subsequent slider edits remain custom. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PSX|General")
	ESimplyPSXPreset Preset = ESimplyPSXPreset::Subtle540;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PSX|Pixelation", meta = (DisplayName = "Enable Pixelation"))
	bool bPixelationEnabled = true;

	/** Virtual vertical resolution. Lower values make larger pixels. Width follows the viewport aspect ratio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PSX|Pixelation", meta = (ClampMin = "64", ClampMax = "4320", UIMin = "120", UIMax = "1080", EditCondition = "bPixelationEnabled"))
	int32 PixelHeight = 540;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PSX|Geometry Wobble", meta = (DisplayName = "Enable Geometry Wobble"))
	bool bWobbleEnabled = true;

	/** Blends between original vertices (0) and fully snapped vertices (1), on compatible PSX materials. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PSX|Geometry Wobble", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bWobbleEnabled"))
	float WobbleStrength = 0.15f;

	/** Vertical resolution of the vertex snapping grid. Independent of screen pixelation; lower is coarser. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PSX|Geometry Wobble", meta = (ClampMin = "64", ClampMax = "4320", UIMin = "120", UIMax = "1080", EditCondition = "bWobbleEnabled"))
	int32 WobbleGridHeight = 540;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PSX|Texture Warping", meta = (DisplayName = "Enable Texture Warping"))
	bool bTextureWarpEnabled = true;

	/** Blends normal perspective-correct textures toward affine PS1-style mapping on compatible materials. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PSX|Texture Warping", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bTextureWarpEnabled"))
	float TextureWarpStrength = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PSX|Color", meta = (DisplayName = "Enable Color Reduction"))
	bool bColorReductionEnabled = false;

	/** Bits per RGB channel: 5 resembles PS1 color precision; 8 preserves most ordinary display colors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PSX|Color", meta = (ClampMin = "2", ClampMax = "8", UIMin = "2", UIMax = "8", EditCondition = "bColorReductionEnabled"))
	int32 ColorBits = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PSX|Color", meta = (DisplayName = "Enable Dithering"))
	bool bDitheringEnabled = false;

	/** Amount of the ordered dither pattern. Most noticeable with color reduction enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PSX|Color", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bDitheringEnabled"))
	float DitherStrength = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "PSX|Assets")
	TObjectPtr<UMaterialInterface> ScreenMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "PSX|Assets")
	TObjectPtr<UMaterialParameterCollection> GeometryParameters;

	/** Read-only status: duplicate controllers are suppressed so the effect is never applied twice. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "PSX|General")
	bool bIsActiveController = false;

	/** Push edited settings to the screen and material effects. Call after runtime property changes. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "PSX")
	void ApplySettings();

	UFUNCTION(BlueprintCallable, Category = "PSX")
	void SetEnabled(bool bNewEnabled);

	/** Apply a complete starting preset once. Custom leaves the current settings intact. */
	UFUNCTION(BlueprintCallable, Category = "PSX")
	void ApplyPreset(ESimplyPSXPreset NewPreset);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "PSX")
	void ApplySelectedPreset();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostRegisterAllComponents() override;
	virtual void PostUnregisterAllComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Destroyed() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditUndo() override;
#endif

private:
	UPROPERTY(VisibleAnywhere, Category = "PSX", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPostProcessComponent> PostProcess;

	UPROPERTY(Transient, DuplicateTransient)
	TObjectPtr<UMaterialInstanceDynamic> ScreenInstance;

	UPROPERTY(Transient, DuplicateTransient)
	TObjectPtr<UMaterialInterface> ScreenInstanceSource;

	UPROPERTY(Transient, DuplicateTransient)
	TObjectPtr<UMaterialParameterCollection> PublishedGeometryParameters;

	bool bRegisteredForEffects = false;
	bool bWarnedMissingScreenMaterial = false;
	bool bWarnedMissingCollection = false;
	bool bWarnedDuplicate = false;

	static void RefreshWorldControllers(UWorld* World);
	void PublishSettings();
	void ReleaseEffects();
	void UnregisterEffects();
};

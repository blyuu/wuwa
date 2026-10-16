#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "LiberationCameraActor.generated.h"

class ACharacter;

/** Samples the animated head after the character mesh has updated its pose. */
UCLASS()
class WUTHERINGWAVES_API ALiberationCameraActor : public ACameraActor
{
	GENERATED_BODY()
public:
	ALiberationCameraActor();
	void Initialize(ACharacter* Character);
	virtual void Tick(float DeltaSeconds) override;
private:
	void UpdateFraming();
	TWeakObjectPtr<ACharacter> Subject;
};

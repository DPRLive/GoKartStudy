// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.h"
#include "GoKartMovementReplicater.generated.h"

USTRUCT()
struct FGoKartState
{
	GENERATED_BODY()

	UPROPERTY()
	FTransform Transform;
	
	UPROPERTY()
	FVector Velocity;

	UPROPERTY()
	FGoKartMove LastMove;
};

struct FHermiteCubicSpline
{
	FVector StartLocation,
	StartDerivative,
	TargetLocation,
	TargetDerivative;

	FVector InterpolateLocation(float LerpRatio) const
	{
		// Cubic Interp : 두 지점에 기울기를 같이 제공하여 두 점 사이를 3차 함수 형태로 보간을 진행? -> 더욱 매끄러운 보간이 가능.?
		return FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	}
	FVector InterpolateDerivative(float LerpRatio) const
	{
		// 해당 지점에서의 기울기 또는 도함수 제공
		return FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	}
	
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartMovementReplicater : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGoKartMovementReplicater();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void ClearAcknowledgeMoves(FGoKartMove LastMove);

	void UpdateServerState(const FGoKartMove& Move);

	void ClientTick(float DeltaTime);
	
	FHermiteCubicSpline CreateSpline();

	void InterpolateLocation(const FHermiteCubicSpline& Spline, float LerpRatio);

	void InterpolateVelocity(const FHermiteCubicSpline& Spline, float LerpRatio);

	void InterpolateRotation(float LerpRatio);
	
	float VelocityToDerivative();
	
	// (_Implementation을)서버에서 실행하라고 요청하는 RPC, 클라이언트에서 실행
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(FGoKartMove Move);

	// 서버에서 받은 표준 상태
	UPROPERTY( ReplicatedUsing = OnRep_ServerState )
	FGoKartState ServerState;
	
	UFUNCTION()
	void OnRep_ServerState();

	void AutonomousProxy_OnRep_ServerState();
	void SimulatedProxy_OnRep_ServerState();
	
	TArray<FGoKartMove> UnacknowledgedMoves;

	float ClientTimeSinceUpdate;
	float ClientTimeBetweenLastUpdates;
	FTransform ClientStartTransform;
	FVector ClientStartVelocity;

	float ClientSimulatedTime;
	
	UPROPERTY()
	UGoKartMovementComponent* MovementComponent;

	// 서버에서의 신뢰성 있는 collision 판정을 위해,
	// 보간은 mesh offset root만 하고 collision은 서버에서의 신뢰된 위치로만 업데이트
	UPROPERTY()
	USceneComponent* MeshOffsetRoot;

	UFUNCTION(BlueprintCallable)
	void SetMeshOffsetRoot(USceneComponent* Root) { MeshOffsetRoot = Root;}
};

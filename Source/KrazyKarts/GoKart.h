// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKartMovementComponent.h"
#include "GoKart.generated.h"

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

UCLASS()
class KRAZYKARTS_API AGoKart : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AGoKart();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	virtual void GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const override;
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	void ClearAcknowledgeMoves(FGoKartMove LastMove);
	
	void MoveForward(float Value);
	void MoveRight(float Value);
	
	// (_Implementation을)서버에서 실행하라고 요청하는 RPC, 클라이언트에서 실행
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(FGoKartMove Move);

	// 서버에서 받은 표준 상태
	UPROPERTY( ReplicatedUsing = OnRep_ServerState )
	FGoKartState ServerState;
	
	UFUNCTION()
	void OnRep_ServerState();
	
	TArray<FGoKartMove> UnacknowledgedMoves;

	UPROPERTY( EditAnywhere )
	UGoKartMovementComponent* MovementComponent;
};
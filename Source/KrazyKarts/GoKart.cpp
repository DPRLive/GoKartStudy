// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"

#include "Components/InputComponent.h"

#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

FString GetEnumText(ENetRole Role)
{
	switch (Role)
	{
	case ROLE_None:
		return TEXT("ROLE_None");
	case ROLE_SimulatedProxy:
		return TEXT("ROLE_SimulatedProxy");
	case ROLE_AutonomousProxy:
		return TEXT("ROLE_AutonomousProxy");
	case ROLE_Authority:
		return TEXT("ROLE_Authority");
	default:
		return TEXT("Error");
	}
}

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

	// 보간을 위해 이동 replicate를 끈다
	SetReplicatingMovement(false);
	
	MovementComponent = CreateDefaultSubobject<UGoKartMovementComponent>(TEXT("MovementComponent"));
	MovementReplicater = CreateDefaultSubobject<UGoKartMovementReplicater>(TEXT("MovementReplicater"));
}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();

	if(HasAuthority())
	{
		NetUpdateFrequency = 1; // 1초에 몇번 업뎃할래":?
	}
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

		
	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(GetLocalRole()), this,
		FColor::Purple, DeltaTime);
}


void AGoKart::MoveForward(float Value)
{
	if(MovementComponent == nullptr)
		return;
	
	// 클라이언트에서 내가 변경하고,
	MovementComponent->SetThrottle(Value);

	// 서버에게 요청한다.(Tick에서)
}

void AGoKart::MoveRight(float Value)
{
	if(MovementComponent == nullptr)
		return;
	
	MovementComponent->SetSteeringThrow(Value);
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);

}


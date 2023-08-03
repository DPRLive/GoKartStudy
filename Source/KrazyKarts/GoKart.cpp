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

void AGoKart::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGoKart, ServerState); // 이러면 서버에서 설정될 때마다 클라이언트들이 받게된다.
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(IsLocallyControlled()) // 로컬이라면
	{
		FGoKartMove Move;
		Move.DeltaTime = DeltaTime; // 설정된 정보들을 담아
		Move.SteeringThrow = SteeringThrow;
		Move.Throttle = Throttle;
		// TODO : SetTime

		Server_SendMove(Move); // 서버에게 새로 만든 move를 보낸다
		
		// 나는 move를 시뮬레이트한다
		SimulateMove(Move);
	}
	
	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(GetLocalRole()), this,
		FColor::Purple, DeltaTime);
}

void AGoKart::OnRep_ServerState()
{
	// 클라이언트는 복제된 위치를 받아다 새로 설정한다.
	SetActorTransform(ServerState.Transform);
	Velocity = ServerState.Velocity;
}


void AGoKart::SimulateMove(FGoKartMove Move)
{
	FVector Force = GetActorForwardVector() * MaxDrivingForce * Move.Throttle;

	Force += GetAirResistance();
	Force += GetRollingResistance();

	FVector Acceleration = Force / Mass;

	Velocity = Velocity + Acceleration * Move.DeltaTime;

	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);

	UpdateLocationFromVelocity(Move.DeltaTime);
}

FVector AGoKart::GetAirResistance()
{
	return - Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
}

FVector AGoKart::GetRollingResistance()
{
	float AccelerationDueToGravity = -GetWorld()->GetGravityZ() / 100;
	float NormalForce = Mass * AccelerationDueToGravity;
	return -Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;
}


void AGoKart::ApplyRotation(float DeltaTime, float InSteeringThrow)
{
	float DeltaLocation = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime;
	float RotationAngle = DeltaLocation / MinTurningRadius * InSteeringThrow;
	FQuat RotationDelta(GetActorUpVector(), RotationAngle);

	Velocity = RotationDelta.RotateVector(Velocity);

	AddActorWorldRotation(RotationDelta);
}

void AGoKart::UpdateLocationFromVelocity(float DeltaTime)
{
	FVector Translation = Velocity * 100 * DeltaTime;

	FHitResult Hit;
	AddActorWorldOffset(Translation, true, &Hit);
	if (Hit.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;
	}
}

void AGoKart::MoveForward(float Value)
{
	// 클라이언트에서 내가 변경하고,
	Throttle = Value;

	// 서버에게 요청한다.(Tick에서)
}

void AGoKart::MoveRight(float Value)
{
	SteeringThrow = Value;
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);

}

// SendMove를 클라에서 호출 시 서버에서 진짜로 실행하는 함수
void AGoKart::Server_SendMove_Implementation(FGoKartMove Move)
{
	// 서버에서 스로틀을 받아와 Tick에서 계산 후 위치를 업데이트 하는데,
	// 서버의 delta time 과 클라이언트의 deltatime은 다르게 흐르기 때문에
	// 위치가 점점 오류가 발생한다.

	SimulateMove(Move);
	// 서버면 위치를 복제하여 클라이언트로 전송한다.
	ServerState.LastMove = Move;
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = Velocity;
	// TODO : Update Last Move
	//
	// Throttle = Move.Throttle;
	// SteeringThrow = Move.SteeringThrow;
}

// 치트방지를 위해 입력값을 검증한다
bool AGoKart::Server_SendMove_Validate(FGoKartMove Move)
{
	return true; // TODO : Make Better Validation
}

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

	MovementComponent = CreateDefaultSubobject<UGoKartMovementComponent>(TEXT("MovementComponent"));
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

	if(MovementComponent == nullptr)
		return;
	
	if( GetLocalRole() == ROLE_AutonomousProxy )
	{
		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);

		// 나는 move를 시뮬레이트한다
		MovementComponent->SimulateMove(Move);
		
		UnacknowledgedMoves.Add(Move);
		Server_SendMove(Move); // 서버에게 새로 만든 move를 보낸다
	}
	// we are the server and in control of the pawn
	if( GetLocalRole() == ROLE_Authority && IsLocallyControlled())
	{
		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
		Server_SendMove(Move);
	}

	if( GetLocalRole() == ROLE_SimulatedProxy )
	{
		MovementComponent->SimulateMove(ServerState.LastMove);
	}
	
	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(GetLocalRole()), this,
		FColor::Purple, DeltaTime);
}

void AGoKart::OnRep_ServerState()
{
	if(MovementComponent == nullptr)
		return;
	
	// 클라이언트는 복제된 위치를 받아다 새로 설정한다.
	SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);

	ClearAcknowledgeMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulateMove(Move);
	}
}

void AGoKart::ClearAcknowledgeMoves(FGoKartMove LastMove)
{
	TArray<FGoKartMove> NewMoves;

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		if(Move.Time > LastMove.Time)
		{
			NewMoves.Add(Move);
		}
	}

	UnacknowledgedMoves = NewMoves;
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

// SendMove를 클라에서 호출 시 서버에서 진짜로 실행하는 함수
void AGoKart::Server_SendMove_Implementation(FGoKartMove Move)
{
	// 서버에서 스로틀을 받아와 Tick에서 계산 후 위치를 업데이트 하는데,
	// 서버의 delta time 과 클라이언트의 deltatime은 다르게 흐르기 때문에
	// 위치가 점점 오류가 발생한다.

	if(MovementComponent == nullptr)
		return;
	
	MovementComponent->SimulateMove(Move);
	// 서버면 위치를 복제하여 클라이언트로 전송한다.
	ServerState.LastMove = Move;
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
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

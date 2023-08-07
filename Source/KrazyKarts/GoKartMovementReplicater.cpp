// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartMovementReplicater.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UGoKartMovementReplicater::UGoKartMovementReplicater()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Replicate ON
	SetIsReplicated(true);
}


// Called when the game starts
void UGoKartMovementReplicater::BeginPlay()
{
	Super::BeginPlay();

	// 리플렉션 좋네
	MovementComponent = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();
}


void UGoKartMovementReplicater::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGoKartMovementReplicater, ServerState); // 이러면 서버에서 설정될 때마다 클라이언트들이 받게된다.
}


// Called every frame
void UGoKartMovementReplicater::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(MovementComponent == nullptr)
		return;

	FGoKartMove LastMove = MovementComponent->GetLastMove();
	
	if( GetOwnerRole() == ROLE_AutonomousProxy )
	{
		UnacknowledgedMoves.Add(LastMove);
		Server_SendMove(LastMove); // 서버에게 새로 만든 move를 보낸다
	}
	// we are the server and in control of the pawn
	if( GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy )
	{
		UpdateServerState(LastMove);
	}

	if( GetOwnerRole() == ROLE_SimulatedProxy )
	{
		ClientTick(DeltaTime);
	}

}


void UGoKartMovementReplicater::UpdateServerState(const FGoKartMove& Move)
{
	// 서버면 위치를 복제하여 클라이언트로 전송한다.
	ServerState.LastMove = Move;
	ServerState.Transform = GetOwner()->GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
}

void UGoKartMovementReplicater::ClientTick(float DeltaTime)
{
	ClientTimeSinceUpdate += DeltaTime;

	if(ClientTimeBetweenLastUpdates < KINDA_SMALL_NUMBER)
		return;

	// 마지막 업데이트가 된 두 사이 시간을 알고 있다
	// 그 시간안에 내가 원하는 목표로 이동해야함
	// 그래서
	// 마지막 업데이트 후 지난 시간 / 그 사이 시간
	// 인건가?
	FVector TargetLocation = ServerState.Transform.GetLocation();
	float LerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdates;
	FVector StartLocation = ClientStartTransform.GetLocation();

	FVector NewLocation = FMath::LerpStable(StartLocation, TargetLocation, LerpRatio);

	GetOwner()->SetActorLocation(NewLocation);

	// 회전은 사원수.. 써야겠지?
	FQuat TargetRotation = ServerState.Transform.GetRotation();
	FQuat StartRotation = ClientStartTransform.GetRotation();

	FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, LerpRatio);

	GetOwner()->SetActorRotation(NewRotation);
}

void UGoKartMovementReplicater::OnRep_ServerState()
{
	switch (GetOwnerRole()) 
	{
	case ROLE_AutonomousProxy:
		AutonomousProxy_OnRep_ServerState();
		break;
	case ROLE_SimulatedProxy:
		SimulatedProxy_OnRep_ServerState();
		break;
	default:
		break;
	}
}

void UGoKartMovementReplicater::AutonomousProxy_OnRep_ServerState()
{
	if(MovementComponent == nullptr)
		return;
	
	// 클라이언트는 복제된 위치를 받아다 새로 설정한다.
	GetOwner()->SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);

	ClearAcknowledgeMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulateMove(Move);
	}
}

void UGoKartMovementReplicater::SimulatedProxy_OnRep_ServerState()
{
	ClientTimeBetweenLastUpdates = ClientTimeSinceUpdate;
	ClientTimeSinceUpdate = 0;

	ClientStartTransform = GetOwner()->GetActorTransform();
}

void UGoKartMovementReplicater::ClearAcknowledgeMoves(FGoKartMove LastMove)
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

// SendMove를 클라에서 호출 시 서버에서 진짜로 실행하는 함수
void UGoKartMovementReplicater::Server_SendMove_Implementation(FGoKartMove Move)
{
	// 서버에서 스로틀을 받아와 Tick에서 계산 후 위치를 업데이트 하는데,
	// 서버의 delta time 과 클라이언트의 deltatime은 다르게 흐르기 때문에
	// 위치가 점점 오류가 발생한다.

	if(MovementComponent == nullptr)
		return;
	
	MovementComponent->SimulateMove(Move);
	UpdateServerState(Move);
}

// 치트방지를 위해 입력값을 검증한다
bool UGoKartMovementReplicater::Server_SendMove_Validate(FGoKartMove Move)
{
	return true; // TODO : Make Better Validation
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "E2GridModifier.h"


// Sets default values
AE2GridModifier::AE2GridModifier()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AE2GridModifier::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AE2GridModifier::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


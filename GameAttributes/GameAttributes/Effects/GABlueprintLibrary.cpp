// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameAttributes.h"
#include "../GAAttributeComponent.h"


#include "GABlueprintLibrary.h"
#include "../IGAAttributes.h"
#include "GABlueprintLibrary.h"
#include "../GAEffectInstanced.h"

UGABlueprintLibrary::UGABlueprintLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

FGAGameEffectHandle UGABlueprintLibrary::MakeOutgoingSpec(FGAGameEffectHandle Handle,
	TSubclassOf<class UGAGameEffectSpec> SpecIn, const FHitResult& Target, APawn* Instigator,
	UObject* Causer, EGAMakeSpecResult& ResultOut)
{
	if(!SpecIn)
	{
		ResultOut = EGAMakeSpecResult::Invalid;
		UE_LOG(GameAttributesEffects, Error, TEXT("Invalid Effect Spec"));
		return FGAGameEffectHandle();
	}
	IIGAAttributes* targetAttr = Cast<IIGAAttributes>(Target.GetActor());
	IIGAAttributes* instiAttr = Cast<IIGAAttributes>(Instigator);
	if (!targetAttr || !instiAttr)
	{
		ResultOut = EGAMakeSpecResult::Invalid;
		UE_LOG(GameAttributesEffects, Error, TEXT("Invalid Target or Instigator"));
		return FGAGameEffectHandle();
	}

	UGAAttributeComponent* targetComp = targetAttr->GetAttributeComponent();
	UGAAttributeComponent* instiComp = instiAttr->GetAttributeComponent();

	FGAEffectContext Context(Target.Location, Target.GetActor(), Causer,
		Instigator, targetComp, instiComp);
	UE_LOG(GameAttributesEffects, Log, TEXT("Created new Context: %s"), *Context.ToString());

	if (Handle.IsValid())
	{
		Handle.SetContext(Context);
		UE_LOG(GameAttributesEffects, Log, TEXT("Set New Context for handle"));
		ResultOut = EGAMakeSpecResult::OldHandle;
		return Handle;
	}

	FGAGameEffectHandle NewHandle = instiComp->MakeGameEffect(SpecIn, Context);
	UE_LOG(GameAttributesEffects, Log, TEXT("Created New Handle"));
	ResultOut = EGAMakeSpecResult::NewHandle;
	return NewHandle;
}

FGAGameEffectHandle UGABlueprintLibrary::MakeOutgoingSpecObj(FGAGameEffectHandle Handle,
	const FGAEffectSpec& SpecIn, const FHitResult& Target, APawn* Instigator,
	UObject* Causer, EGAMakeSpecResult& ResultOut)
{
	if (!SpecIn.Spec)
	{
		ResultOut = EGAMakeSpecResult::Invalid;
		UE_LOG(GameAttributesEffects, Error, TEXT("Invalid Effect Spec"));
		return FGAGameEffectHandle();
	}
	IIGAAttributes* targetAttr = Cast<IIGAAttributes>(Target.GetActor());
	IIGAAttributes* instiAttr = Cast<IIGAAttributes>(Instigator);
	if (!targetAttr || !instiAttr)
	{
		ResultOut = EGAMakeSpecResult::Invalid;
		UE_LOG(GameAttributesEffects, Error, TEXT("Invalid Target or Instigator"));
		return FGAGameEffectHandle();
	}

	UGAAttributeComponent* targetComp = targetAttr->GetAttributeComponent();
	UGAAttributeComponent* instiComp = instiAttr->GetAttributeComponent();

	FGAEffectContext Context(Target.Location, Target.GetActor(), Causer,
		Instigator, targetComp, instiComp);
	UE_LOG(GameAttributesEffects, Log, TEXT("MakeOutgoingSpecObj: Created new Context: %s"), *Context.ToString());

	if (Handle.IsValid())
	{
		Handle.SetContext(Context);
		UE_LOG(GameAttributesEffects, Log, TEXT("Set New Context for handle"));
		ResultOut = EGAMakeSpecResult::OldHandle;
		return Handle;
	}

	//FGAGameEffectHandle NewHandle = instiComp->MakeGameEffect(SpecIn, Context);
	FGAGameEffect* effect = new FGAGameEffect(SpecIn.Spec, Context);
	FGAGameEffectHandle handle = FGAGameEffectHandle::GenerateHandle(effect);
	effect->Handle = &handle;

	UE_LOG(GameAttributesEffects, Log, TEXT("Created New Handle"));
	ResultOut = EGAMakeSpecResult::NewHandle;
	return handle;
}

void UGABlueprintLibrary::ApplyGameEffect(FGAGameEffectHandle Handle)
{
	if (!Handle.IsValid())
		return;

	FGAEffectContext& Context = Handle.GetContext();

	Context.InstigatorComp->ApplyEffectToTarget(Handle.GetEffect(), Handle);
}

void UGABlueprintLibrary::ApplyGameEffectInstance(TSubclassOf<class UGAEffectInstanced> EffectClass,
	const FHitResult& Target, APawn* Instigator,
	UObject* Causer, EGAMakeSpecResult& ResultOu)
{
	UGAEffectInstanced* EffectCDO = EffectClass.GetDefaultObject();
	//do not apply instanced effects with duration less than zero.
	if (EffectCDO && EffectCDO->Duration <= 0)
		return;

	FGAEffectContext Context =  MakeHitContext(Target, Instigator, Causer);
	
	if (!Context.IsValid())
		return;

	UGAEffectInstanced* Effect = NewObject<UGAEffectInstanced>(Context.TargetComp.Get(), EffectClass);

	if (Effect)
	{
		Effect->Context = Context;
	}
	Context.InstigatorComp->ApplyInstancedToTarget(Effect);
}

FGAGameEffectHandle UGABlueprintLibrary::ApplyGameEffectToActor(const FGAEffectSpec& SpecIn,
	FGAGameEffectHandle HandleIn, class AActor* Target, class APawn* Instigator,
	UObject* Causer)
{
	if (!SpecIn.Spec)
	{
		UE_LOG(GameAttributesEffects, Error, TEXT("Invalid Effect Spec"));
		return FGAGameEffectHandle();
	}
	FGAEffectContext Context = MakeActorContext(Target, Instigator, Causer);
	if (!Context.IsValid())
		return FGAGameEffectHandle();

	UE_LOG(GameAttributesEffects, Log, TEXT("MakeOutgoingSpecObj: Created new Context: %s"), *Context.ToString());

	if (HandleIn.IsValid())
	{
		HandleIn.SetContext(Context);
	}
	else
	{
		FGAGameEffect* effect = new FGAGameEffect(SpecIn.Spec, Context);
		HandleIn = FGAGameEffectHandle::GenerateHandle(effect);
		effect->Handle = &HandleIn;
	}
	Context.InstigatorComp->ApplyEffectToTarget(HandleIn.GetEffect(), HandleIn);
	return HandleIn;
}

FGAGameEffectHandle UGABlueprintLibrary::ApplyGameEffectToActorFromClass(TSubclassOf<class UGAGameEffectSpec> SpecIn,
	FGAGameEffectHandle HandleIn, class AActor* Target, class APawn* Instigator,
	UObject* Causer)
{
	if (SpecIn)
	{
		UE_LOG(GameAttributesEffects, Error, TEXT("Invalid Effect Spec"));
		return FGAGameEffectHandle();
	}

	FGAEffectContext Context = MakeActorContext(Target, Instigator, Causer);
	if (!Context.IsValid())
		return FGAGameEffectHandle();

	UE_LOG(GameAttributesEffects, Log, TEXT("MakeOutgoingSpecObj: Created new Context: %s"), *Context.ToString());

	if (HandleIn.IsValid())
	{
		HandleIn.SetContext(Context);
	}
	else
	{
		UGAGameEffectSpec* EffectCDO = SpecIn.GetDefaultObject();
		FGAGameEffect* effect = new FGAGameEffect(EffectCDO, Context);
		HandleIn = FGAGameEffectHandle::GenerateHandle(effect);
		effect->Handle = &HandleIn;
	}
	Context.InstigatorComp->ApplyEffectToTarget(HandleIn.GetEffect(), HandleIn);
	return HandleIn;
}

FGAEffectContext UGABlueprintLibrary::MakeActorContext(class AActor* Target, class APawn* Instigator,
	UObject* Causer)
{
	IIGAAttributes* targetAttr = Cast<IIGAAttributes>(Target);
	IIGAAttributes* instiAttr = Cast<IIGAAttributes>(Instigator);
	if (!targetAttr || !instiAttr)
	{
		UE_LOG(GameAttributesEffects, Error, TEXT("Invalid Target or Instigator"));
		return FGAEffectContext();
	}

	UGAAttributeComponent* targetComp = targetAttr->GetAttributeComponent();
	UGAAttributeComponent* instiComp = instiAttr->GetAttributeComponent();
	FVector location = Target->GetActorLocation();
	FGAEffectContext Context(location, Target, Causer,
		Instigator, targetComp, instiComp);

	return Context;
}

FGAEffectContext UGABlueprintLibrary::MakeHitContext(const FHitResult& Target, class APawn* Instigator, UObject* Causer)
{
	IIGAAttributes* targetAttr = Cast<IIGAAttributes>(Target.GetActor());
	IIGAAttributes* instiAttr = Cast<IIGAAttributes>(Instigator);
	if (!targetAttr || !instiAttr)
	{
		UE_LOG(GameAttributesEffects, Error, TEXT("Invalid Target or Instigator"));
		return FGAEffectContext();
	}

	UGAAttributeComponent* targetComp = targetAttr->GetAttributeComponent();
	UGAAttributeComponent* instiComp = instiAttr->GetAttributeComponent();
	
	FGAEffectContext Context(Target.Location, Target.GetActor(), Causer,
		Instigator, targetComp, instiComp);

	return Context;
}
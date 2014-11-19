// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagAssetInterface.h"
#include "AttributeSet.h"
#include "GameplayPrediction.h"
#include "GameplayEffectTypes.generated.h"

#define SKILL_SYSTEM_AGGREGATOR_DEBUG 1

#if SKILL_SYSTEM_AGGREGATOR_DEBUG
	#define SKILL_AGG_DEBUG( Format, ... ) *FString::Printf(Format, ##__VA_ARGS__)
#else
	#define SKILL_AGG_DEBUG( Format, ... ) NULL
#endif

class UAbilitySystemComponent;
class UGameplayEffect;

struct FGameplayEffectSpec;
struct FGameplayEffectModCallbackData;

FString EGameplayModOpToString(int32 Type);

FString EGameplayModToString(int32 Type);

FString EGameplayModEffectToString(int32 Type);

FString EGameplayEffectCopyPolicyToString(int32 Type);

FString EGameplayEffectStackingPolicyToString(int32 Type);

UENUM(BlueprintType)
namespace EGameplayModOp
{
	enum Type
	{		
		// Numeric
		Additive = 0		UMETA(DisplayName="Add"),
		Multiplicitive		UMETA(DisplayName="Multiply"),
		Division			UMETA(DisplayName="Divide"),

		// Other
		Override 			UMETA(DisplayName="Override"),	// This should always be the first non numeric ModOp

		// This must always be at the end
		Max					UMETA(DisplayName="Invalid")
	};
}

/**
 * Tells us how to handle copying gameplay effect when it is applied.
 *	Default means to use context - e.g, OutgoingGE is are always snapshots, IncomingGE is always Link
 *	AlwaysSnapshot vs AlwaysLink let mods themselves override
 */
UENUM(BlueprintType)
namespace EGameplayEffectCopyPolicy
{
	enum Type
	{
		Default = 0			UMETA(DisplayName="Default"),
		AlwaysSnapshot		UMETA(DisplayName="AlwaysSnapshot"),
		AlwaysLink			UMETA(DisplayName="AlwaysLink")
	};
}

UENUM(BlueprintType)
namespace EGameplayEffectStackingPolicy
{
	enum Type
	{
		Unlimited = 0		UMETA(DisplayName = "NoRule"),
		Highest				UMETA(DisplayName = "Strongest"),
		Lowest				UMETA(DisplayName = "Weakest"),
		Replaces			UMETA(DisplayName = "MostRecent"),
		Callback			UMETA(DisplayName = "Custom"),

		// This must always be at the end
		Max					UMETA(DisplayName = "Invalid")
	};
}

/** Enumeration for options of where to capture gameplay attributes from for gameplay effects */
UENUM()
enum class EGameplayEffectAttributeCaptureSource : uint8
{
	Source,	// Source (caster) of the gameplay effect
	Target	// Target (recipient) of the gameplay effect
};

/**
 * This handle is required for things outside of FActiveGameplayEffectsContainer to refer to a specific active GameplayEffect
 *	For example if a skill needs to create an active effect and then destroy that specific effect that it created, it has to do so
 *	through a handle. a pointer or index into the active list is not sufficient.
 */
USTRUCT(BlueprintType)
struct GAMEPLAYABILITIES_API FActiveGameplayEffectHandle
{
	GENERATED_USTRUCT_BODY()

	FActiveGameplayEffectHandle()
		: Handle(INDEX_NONE)
	{

	}

	FActiveGameplayEffectHandle(int32 InHandle)
		: Handle(InHandle)
	{

	}

	bool IsValid() const
	{
		return Handle != INDEX_NONE;
	}

	static FActiveGameplayEffectHandle GenerateNewHandle(UAbilitySystemComponent* OwningComponent);

	UAbilitySystemComponent* GetOwningAbilitySystemComponent();
	const UAbilitySystemComponent* GetOwningAbilitySystemComponent() const;

	void RemoveFromGlobalMap();

	bool operator==(const FActiveGameplayEffectHandle& Other) const
	{
		return Handle == Other.Handle;
	}

	bool operator!=(const FActiveGameplayEffectHandle& Other) const
	{
		return Handle != Other.Handle;
	}

	friend uint32 GetTypeHash(const FActiveGameplayEffectHandle& InHandle)
	{
		return InHandle.Handle;
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("%d"), Handle);
	}

private:

	UPROPERTY()
	int32 Handle;
};

USTRUCT()
struct FGameplayModifierEvaluatedData
{
	GENERATED_USTRUCT_BODY()

	FGameplayModifierEvaluatedData()
		: Attribute()
		, ModifierOp(EGameplayModOp::Additive)
		, Magnitude(0.f)
		, IsValid(false)
	{
	}

	FGameplayModifierEvaluatedData(const FGameplayAttribute& InAttribute, TEnumAsByte<EGameplayModOp::Type> InModOp, float InMagnitude, FActiveGameplayEffectHandle InHandle = FActiveGameplayEffectHandle())
		: Attribute(InAttribute)
		, ModifierOp(InModOp)
		, Magnitude(InMagnitude)
		, Handle(InHandle)
		, IsValid(true)
	{
	}

	UPROPERTY()
	FGameplayAttribute Attribute;

	/** The numeric operation of this modifier: Override, Add, Multiply, etc  */
	UPROPERTY()
	TEnumAsByte<EGameplayModOp::Type> ModifierOp;

	UPROPERTY()
	float Magnitude;

	/** Handle of the active gameplay effect that originated us. Will be invalid in many cases */
	UPROPERTY()
	FActiveGameplayEffectHandle	Handle;

	UPROPERTY()
	bool IsValid;

	FString ToSimpleString() const
	{
		return FString::Printf(TEXT("%s %s EvalMag: %f"), *Attribute.GetName(), *EGameplayModOpToString(ModifierOp), Magnitude);
	}
};

/** Struct defining gameplay attribute capture options for gameplay effects */
USTRUCT()
struct GAMEPLAYABILITIES_API FGameplayEffectAttributeCaptureDefinition
{
	GENERATED_USTRUCT_BODY()

	FGameplayEffectAttributeCaptureDefinition()
	{

	}

	FGameplayEffectAttributeCaptureDefinition(FGameplayAttribute InAttribute, EGameplayEffectAttributeCaptureSource InSource, bool InSnapshot)
		: AttributeToCapture(InAttribute), AttributeSource(InSource), bSnapshot(InSnapshot)
	{

	}

	/** Gameplay attribute to capture */
	UPROPERTY(EditDefaultsOnly, Category=Capture)
	FGameplayAttribute AttributeToCapture;

	/** Source of the gameplay attribute */
	UPROPERTY(EditDefaultsOnly, Category=Capture)
	EGameplayEffectAttributeCaptureSource AttributeSource;

	/** Whether the attribute should be snapshotted or not */
	UPROPERTY(EditDefaultsOnly, Category=Capture)
	bool bSnapshot;

	/** Equality/Inequality operators */
	bool operator==(const FGameplayEffectAttributeCaptureDefinition& Other) const;
	bool operator!=(const FGameplayEffectAttributeCaptureDefinition& Other) const;

	/**
	 * Get type hash for the capture definition; Implemented to allow usage in TMap
	 *
	 * @param CaptureDef Capture definition to get the type hash of
	 */
	friend uint32 GetTypeHash(const FGameplayEffectAttributeCaptureDefinition& CaptureDef)
	{
		return FCrc::MemCrc32(&CaptureDef, sizeof(FGameplayEffectAttributeCaptureDefinition));
	}

	FString ToSimpleString() const;
};

/**
 * FGameplayEffectContext
 *	Data struct for an instigator and related data. This is still being fleshed out. We will want to track actors but also be able to provide some level of tracking for actors that are destroyed.
 *	We may need to store some positional information as well.
 */
USTRUCT()
struct GAMEPLAYABILITIES_API FGameplayEffectContext
{
	GENERATED_USTRUCT_BODY()

	FGameplayEffectContext()
		: Instigator(NULL)
		, EffectCauser(NULL)
		, InstigatorAbilitySystemComponent(NULL)
		, bHasWorldOrigin(false)
	{
	}

	FGameplayEffectContext(AActor* InInstigator, AActor* InEffectCauser)
		: Instigator(NULL)
		, EffectCauser(NULL)
		, InstigatorAbilitySystemComponent(NULL)
	{
		AddInstigator(InInstigator, InEffectCauser);
	}

	virtual ~FGameplayEffectContext()
	{
	}

	/** Returns the list of gameplay tags applicable to this effect, defaults to the owner's tags */
	virtual void GetOwnedGameplayTags(OUT FGameplayTagContainer &TagContainer) const;

	/** Sets the instigator and effect causer. Instigator is who owns the ability that spawned this, EffectCauser is the actor that is the physical source of the effect, such as a weapon. They can be the same. */
	virtual void AddInstigator(class AActor *InInstigator, class AActor *InEffectCauser);

	/** Returns the immediate instigator that applied this effect */
	virtual AActor* GetInstigator() const
	{
		return Instigator;
	}

	/** Returns the ability system component of the instigator of this effect */
	virtual UAbilitySystemComponent* GetInstigatorAbilitySystemComponent() const
	{
		return InstigatorAbilitySystemComponent;
	}

	/** Returns the physical actor tied to the application of this effect */
	virtual AActor* GetEffectCauser() const
	{
		return EffectCauser;
	}

	/** Should always return the original instigator that started the whole chain. Subclasses can override what this does */
	virtual AActor* GetOriginalInstigator() const
	{
		return Instigator;
	}

	/** Returns the ability system component of the instigator that started the whole chain */
	virtual UAbilitySystemComponent* GetOriginalInstigatorAbilitySystemComponent() const
	{
		return InstigatorAbilitySystemComponent;
	}

	virtual void AddActors(TArray<TWeakObjectPtr<AActor>> InActor, bool bReset = false);

	virtual void AddHitResult(const FHitResult InHitResult, bool bReset = false);

	virtual const TArray<TWeakObjectPtr<AActor>> GetActors() const
	{
		return Actors;
	}

	virtual const FHitResult* GetHitResult() const
	{
		if (HitResult.IsValid())
		{
			return HitResult.Get();
		}
		return NULL;
	}

	virtual void AddOrigin(FVector InOrigin);

	virtual const FVector& GetOrigin() const
	{
		return WorldOrigin;
	}

	virtual bool HasOrigin() const
	{
		return bHasWorldOrigin;
	}

	virtual FString ToString() const
	{
		return Instigator ? Instigator->GetName() : FString(TEXT("NONE"));
	}

	virtual UScriptStruct* GetScriptStruct() const
	{
		return FGameplayEffectContext::StaticStruct();
	}

	/** Creates a copy of this context, used to duplicate for later modifications */
	virtual FGameplayEffectContext* Duplicate() const
	{
		FGameplayEffectContext* NewContext = new FGameplayEffectContext();
		*NewContext = *this;
		NewContext->AddActors(Actors);
		if (GetHitResult())
		{
			// Does a deep copy of the hit result
			NewContext->AddHitResult(*GetHitResult());
		}
		return NewContext;
	}

	virtual bool IsLocallyControlled() const;

	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

protected:

	/** Instigator actor, the actor that owns the ability system component */
	UPROPERTY()
	AActor* Instigator;

	/** The physical actor that actually did the damage, can be a weapon or projectile */
	UPROPERTY()
	AActor* EffectCauser;

	/** The ability system component that's bound to instigator */
	UPROPERTY(NotReplicated)
	UAbilitySystemComponent* InstigatorAbilitySystemComponent;

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> Actors;

	/** Trace information - may be NULL in many cases */
	TSharedPtr<FHitResult>	HitResult;

	UPROPERTY()
	FVector	WorldOrigin;

	UPROPERTY()
	bool bHasWorldOrigin;
};

template<>
struct TStructOpsTypeTraits< FGameplayEffectContext > : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true		// Necessary so that TSharedPtr<FHitResult> Data is copied around
	};
};

/**
 * Handle that wraps a FGameplayEffectContext or subclass, to allow it to be polymorphic and replicate properly
 */
USTRUCT(BlueprintType)
struct FGameplayEffectContextHandle
{
	GENERATED_USTRUCT_BODY()

	FGameplayEffectContextHandle()
	{
	}

	/** Constructs from an existing context, should be allocated by new */
	FGameplayEffectContextHandle(FGameplayEffectContext* DataPtr)
	{
		Data = TSharedPtr<FGameplayEffectContext>(DataPtr);
	}

	/** Sets from an existing context, should be allocated by new */
	void operator=(FGameplayEffectContext* DataPtr)
	{
		Data = TSharedPtr<FGameplayEffectContext>(DataPtr);
	}

	void Clear()
	{
		Data.Reset();
	}

	bool IsValid() const
	{
		return Data.IsValid();
	}

	FGameplayEffectContext* Get()
	{
		return IsValid() ? Data.Get() : NULL;
	}

	const FGameplayEffectContext* Get() const
	{
		return IsValid() ? Data.Get() : NULL;
	}

	/** Returns the list of gameplay tags applicable to this effect, defaults to the owner's tags */
	void GetOwnedGameplayTags(OUT FGameplayTagContainer &TagContainer) const
	{
		if (IsValid())
		{
			Data->GetOwnedGameplayTags(TagContainer);
		}
	}

	/** Sets the instigator and effect causer. Instigator is who owns the ability that spawned this, EffectCauser is the actor that is the physical source of the effect, such as a weapon. They can be the same. */
	void AddInstigator(class AActor *InInstigator, class AActor *InEffectCauser)
	{
		if (IsValid())
		{
			Data->AddInstigator(InInstigator, InEffectCauser);
		}
	}

	/** Returns the immediate instigator that applied this effect */
	virtual AActor* GetInstigator() const
	{
		if (IsValid())
		{
			return Data->GetInstigator();
		}
		return NULL;
	}

	/** Returns the ability system component of the instigator of this effect */
	virtual UAbilitySystemComponent* GetInstigatorAbilitySystemComponent() const
	{
		if (IsValid())
		{
			return Data->GetInstigatorAbilitySystemComponent();
		}
		return NULL;
	}

	/** Returns the physical actor tied to the application of this effect */
	virtual AActor* GetEffectCauser() const
	{
		if (IsValid())
		{
			return Data->GetEffectCauser();
		}
		return NULL;
	}

	/** Should always return the original instigator that started the whole chain. Subclasses can override what this does */
	AActor* GetOriginalInstigator() const
	{
		if (IsValid())
		{
			return Data->GetOriginalInstigator();
		}
		return NULL;
	}

	/** Returns the ability system component of the instigator that started the whole chain */
	UAbilitySystemComponent* GetOriginalInstigatorAbilitySystemComponent() const
	{
		if (IsValid())
		{
			return Data->GetOriginalInstigatorAbilitySystemComponent();
		}
		return NULL;
	}

	/** Returns if the instigator is locally controlled */
	bool IsLocallyControlled() const
	{
		if (IsValid())
		{
			return Data->IsLocallyControlled();
		}
		return false;
	}

	void AddActors(TArray<TWeakObjectPtr<AActor>> InActors, bool bReset = false)
	{
		if (IsValid())
		{
			Data->AddActors(InActors, bReset);
		}
	}

	void AddHitResult(const FHitResult InHitResult, bool bReset = false)
	{
		if (IsValid())
		{
			Data->AddHitResult(InHitResult, bReset);
		}
	}

	TArray<TWeakObjectPtr<AActor>> GetActors()
	{
		return Data->GetActors();
	}

	const FHitResult* GetHitResult() const
	{
		if (IsValid())
		{
			return Data->GetHitResult();
		}
		return NULL;
	}

	void AddOrigin(FVector InOrigin)
	{
		if (IsValid())
		{
			Data->AddOrigin(InOrigin);
		}
	}

	virtual const FVector& GetOrigin() const
	{
		if (IsValid())
		{
			return Data->GetOrigin();
		}
		return FVector::ZeroVector;
	}

	virtual bool HasOrigin() const
	{
		if (IsValid())
		{
			return Data->HasOrigin();
		}
		return false;
	}

	FString ToString() const
	{
		return IsValid() ? Data->ToString() : FString(TEXT("NONE"));
	}

	/** Creates a deep copy of this handle, used before modifying */
	FGameplayEffectContextHandle Duplicate() const
	{
		if (IsValid())
		{
			FGameplayEffectContext* NewContext = Data->Duplicate();
			return FGameplayEffectContextHandle(NewContext);
		}
		else
		{
			return FGameplayEffectContextHandle();
		}
	}

	/** Comparison operator */
	bool operator==(FGameplayEffectContextHandle const& Other) const
	{
		if (Data.IsValid() != Other.Data.IsValid())
		{
			return false;
		}
		if (Data.Get() != Other.Data.Get())
		{
			return false;
		}
		return true;
	}

	/** Comparison operator */
	bool operator!=(FGameplayEffectContextHandle const& Other) const
	{
		return !(FGameplayEffectContextHandle::operator==(Other));
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

private:

	TSharedPtr<FGameplayEffectContext> Data;
};

template<>
struct TStructOpsTypeTraits<FGameplayEffectContextHandle> : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithCopy = true,		// Necessary so that TSharedPtr<FGameplayEffectContext> Data is copied around
		WithNetSerializer = true,
		WithIdenticalViaEquality = true,
	};
};

// -----------------------------------------------------------


USTRUCT(BlueprintType)
struct FGameplayCueParameters
{
	GENERATED_USTRUCT_BODY()

	/** Magnitude of source gameplay effect, normalzed from 0-1. Use this for "how strong is the gameplay effet" (0=min, 1=,max) */
	UPROPERTY(BlueprintReadWrite, Category=GameplayCue)
	float NormalizedMagnitude;

	/** Raw final magnitude of source gameplay effect. Use this is you need to display numbers or for other informational purposes. */
	UPROPERTY(BlueprintReadWrite, Category=GameplayCue)
	float RawMagnitude;

	UPROPERTY()
	FGameplayEffectContextHandle EffectContext;

	UPROPERTY(BlueprintReadWrite, Category=GameplayCue)
	FName MatchedTagName;

	UPROPERTY(BlueprintReadWrite, Category=GameplayCue)
	FGameplayTag OriginalTag;
};

UENUM(BlueprintType)
namespace EGameplayCueEvent
{
	enum Type
	{
		OnActive,		// Called when GameplayCue is activated.
		WhileActive,	// Called when GameplayCue is active, even if it wasn't actually just applied (Join in progress, etc)
		Executed,		// Called when a GameplayCue is executed: instant effects or periodic tick
		Removed			// Called when GameplayCue is removed
	};
}

DECLARE_DELEGATE_OneParam(FOnGameplayAttributeEffectExecuted, struct FGameplayModifierEvaluatedData&);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGameplayEffectTagCountChanged, const FGameplayTag, int32 );

DECLARE_MULTICAST_DELEGATE(FOnActiveGameplayEffectRemoved);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGameplayAttributeChange, float ,const FGameplayEffectModCallbackData*);

DECLARE_DELEGATE_RetVal(FGameplayTagContainer, FGetGameplayTags);

DECLARE_DELEGATE_RetVal_OneParam(FOnGameplayEffectTagCountChanged&, FRegisterGameplayTagChangeDelegate, FGameplayTag);

// -----------------------------------------------------------

/** 
 *	Structure that contains a counted set of GameplayTags. Can optionally include parent tags
 *	
 */
struct FGameplayTagCountContainer
{
	FGameplayTagCountContainer()
	: TagContainerType(EGameplayTagMatchType::Explicit)
	{ }

	FGameplayTagCountContainer(EGameplayTagMatchType::Type InTagContainerType)
	: TagContainerType(InTagContainerType)
	{ }

	bool HasMatchingGameplayTag(FGameplayTag TagToCheck, EGameplayTagMatchType::Type TagMatchType) const;
	bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer, EGameplayTagMatchType::Type TagMatchType, bool bCountEmptyAsMatch = true) const;
	bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer, EGameplayTagMatchType::Type TagMatchType, bool bCountEmptyAsMatch = true) const;
	void UpdateTagMap(const struct FGameplayTagContainer& Container, int32 CountDelta);
	void UpdateTagMap(const struct FGameplayTag& Tag, int32 CountDelta);

	TMap<struct FGameplayTag, FOnGameplayEffectTagCountChanged> GameplayTagEventMap;
	TMap<struct FGameplayTag, int32> GameplayTagCountMap;

	/** This is called when any tag is added new or removed completely (going too or from 0 count). Not called for other count increases (e.g, going from 2-3 count) */
	FOnGameplayEffectTagCountChanged	OnAnyTagChangeDelegate;

	EGameplayTagMatchType::Type TagContainerType;

private:

	// Fixme: This may not be adding tag parents correctly. The TagContainer version of this function properly adds parent tags
	void UpdateTagMap_Internal(const struct FGameplayTag& Tag, int32 CountDelta);
};

// -----------------------------------------------------------

/** Encapsulate require and ignore tags */
USTRUCT(BlueprintType)
struct FGameplayTagRequirements
{
	GENERATED_USTRUCT_BODY()

	/** All of these tags must be present */
	UPROPERTY(EditDefaultsOnly, Category = GameplayModifier)
	FGameplayTagContainer RequireTags;

	/** None of these tags may be present */
	UPROPERTY(EditDefaultsOnly, Category = GameplayModifier)
	FGameplayTagContainer IgnoreTags;

	bool	RequirementsMet(const FGameplayTagContainer& Container) const;
	bool	IsEmpty() const;

	static FGetGameplayTags	SnapshotTags(FGetGameplayTags TagDelegate);

	FString ToString() const;
};

USTRUCT()
struct GAMEPLAYABILITIES_API FTagContainerAggregator
{
	GENERATED_USTRUCT_BODY()

	FTagContainerAggregator() : CacheIsValid(false) {}

	FGameplayTagContainer& GetActorTags();
	const FGameplayTagContainer& GetActorTags() const;

	FGameplayTagContainer& GetSpecTags();
	const FGameplayTagContainer& GetSpecTags() const;



	const FGameplayTagContainer* GetAggregatedTags() const;

private:

	UPROPERTY()
	FGameplayTagContainer CapturedActorTags;

	UPROPERTY()
	FGameplayTagContainer CapturedSpecTags;

	UPROPERTY()
	FGameplayTagContainer ScopedTags;

	mutable FGameplayTagContainer CachedAggregator;
	mutable bool CacheIsValid;
};

// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "FootholdTraceLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "FootholdLadderComponent.h"
#include "FootholdSharedSettings.h"
#include "FootholdWorldSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "MoveLibrary/MovementUtilsTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FootholdTraceLibrary)

namespace UE::Foothold::Trace
{
	static TAutoConsoleVariable<bool> CVarDebugDraw(TEXT("foothold.Debug"), false, TEXT("Draw Foothold probes, ledges and anchors."));

	// How far above the highest ledge we look for the top, and below the lowest one.
	constexpr float TopProbeMargin = 25.f;
	// The ledge must still be there this far behind the edge (cm, before MinLedgeDepth).
	constexpr float EdgeInset = 8.f;
	// The capsule stands this far behind the edge on top.
	constexpr float StandInset = 6.f;
	// Ledge probes start this far below the found top to read the wall normal just under the edge.
	constexpr float UnderEdgeProbe = 6.f;
	// Height of the lowest wall probe above the feet: keeps the probe off the floor and small steps.
	constexpr float LowestWallProbe = 25.f;
	// Step used to lower the top probe's start out of a ceiling.
	constexpr float TopStartStep = 10.f;

	static bool ShouldDraw(const FFootholdQueryContext& Ctx)
	{
		return CVarDebugDraw.GetValueOnGameThread() || (Ctx.Settings && Ctx.Settings->bDrawDebug);
	}

	static FVector Planar(const FVector& V, const FVector& Up)
	{
		return FVector::VectorPlaneProject(V, Up).GetSafeNormal();
	}
}

// FFootholdQueryContext ---------------------------------------------------------------

FQuat FFootholdQueryContext::GetCapsuleRotation() const
{
	return FQuat::FindBetweenNormals(FVector::UpVector, Up);
}

ECollisionChannel FFootholdQueryContext::GetTraceChannel() const
{
	return Settings ? Settings->TraceChannel.GetValue() : ECC_Visibility;
}

FCollisionQueryParams FFootholdQueryContext::MakeQueryParams(FName TraceTag) const
{
	FCollisionQueryParams Params(TraceTag, SCENE_QUERY_STAT_ONLY(FootholdTrace), /*bTraceComplex*/ false);
	if (IgnoreActor)
	{
		Params.AddIgnoredActor(IgnoreActor);
	}
	return Params;
}

FFootholdQueryContext FFootholdQueryContext::Make(UWorld* World, const FVector& Location, const FVector& Up, const FVector& Forward,
	float CapsuleRadius, float CapsuleHalfHeight, const UFootholdSharedSettings* Settings, const AActor* IgnoreActor)
{
	FFootholdQueryContext Ctx;
	Ctx.World = World;
	Ctx.Location = Location;
	Ctx.Up = Up.GetSafeNormal();
	Ctx.Forward = UE::Foothold::Trace::Planar(Forward, Ctx.Up);
	Ctx.CapsuleRadius = CapsuleRadius;
	Ctx.CapsuleHalfHeight = CapsuleHalfHeight;
	Ctx.Settings = Settings;
	Ctx.IgnoreActor = IgnoreActor;
	Ctx.ClearanceResponses = FCollisionResponseParams::DefaultResponseParam;
	return Ctx;
}

FFootholdQueryContext FFootholdQueryContext::Make(const FMovingComponentSet& MovingComps, const UFootholdSharedSettings* Settings, const FVector& Up)
{
	const USceneComponent* Updated = MovingComps.UpdatedComponent.Get();
	const UPrimitiveComponent* Primitive = MovingComps.UpdatedPrimitive.Get();

	float Radius = 34.f;
	float HalfHeight = 88.f;
	if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Primitive))
	{
		Capsule->GetScaledCapsuleSize(Radius, HalfHeight);
	}
	else if (Primitive)
	{
		const FBoxSphereBounds Bounds = Primitive->CalcBounds(Primitive->GetComponentTransform());
		Radius = FMath::Min(Bounds.BoxExtent.X, Bounds.BoxExtent.Y);
		HalfHeight = Bounds.BoxExtent.Z;
	}

	FFootholdQueryContext Ctx = Make(Updated ? Updated->GetWorld() : nullptr,
		Updated ? Updated->GetComponentLocation() : FVector::ZeroVector,
		Up,
		Updated ? Updated->GetForwardVector() : FVector::ForwardVector,
		Radius, HalfHeight, Settings, Updated ? Updated->GetOwner() : nullptr);

	if (Primitive)
	{
		Ctx.ClearanceChannel = Primitive->GetCollisionObjectType();
		Ctx.ClearanceResponses = FCollisionResponseParams(Primitive->GetCollisionResponseToChannels());
	}
	return Ctx;
}

// Library -----------------------------------------------------------------------------

bool UFootholdTraceLibrary::HasTag(const FHitResult& Hit, FName Tag)
{
	if (Tag.IsNone())
	{
		return false;
	}
	if (const UPrimitiveComponent* Comp = Hit.GetComponent())
	{
		if (Comp->ComponentHasTag(Tag))
		{
			return true;
		}
	}
	if (const AActor* Actor = Hit.GetActor())
	{
		return Actor->ActorHasTag(Tag);
	}
	return false;
}

bool UFootholdTraceLibrary::IsWalkable(const FVector& Normal, const FVector& Up, float MaxWalkSlopeCosine)
{
	return FVector::DotProduct(Normal, Up) >= MaxWalkSlopeCosine - KINDA_SMALL_NUMBER;
}

bool UFootholdTraceLibrary::IsClimbable(const FHitResult& Hit, const UFootholdSharedSettings& Settings, const FVector& Up)
{
	if (HasTag(Hit, Settings.NoClimbTag))
	{
		return false;
	}
	const float UpDot = FVector::DotProduct(Hit.ImpactNormal, Up);
	if (FMath::Abs(UpDot) > Settings.GetWallUpDotMax())
	{
		return false;
	}
	return Settings.ClimbPolicy == EFootholdClimbablePolicy::AnySteepSurface || HasTag(Hit, Settings.ClimbableTag);
}

bool UFootholdTraceLibrary::IsCapsuleClear(const FFootholdQueryContext& Ctx, const FVector& Center, float Inflate)
{
	if (!Ctx.World)
	{
		return false;
	}
	const float Radius = FMath::Max(1.f, Ctx.CapsuleRadius + Inflate);
	const float HalfHeight = FMath::Max(Radius, Ctx.CapsuleHalfHeight + Inflate);
	const FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
	const bool bBlocked = Ctx.World->OverlapBlockingTestByChannel(Center, Ctx.GetCapsuleRotation(), Ctx.ClearanceChannel, Shape,
		Ctx.MakeQueryParams(TEXT("FootholdClearance")), Ctx.ClearanceResponses);

	if (UE::Foothold::Trace::ShouldDraw(Ctx))
	{
		DrawDebugCapsule(Ctx.World, Center, HalfHeight, Radius, Ctx.GetCapsuleRotation(), bBlocked ? FColor::Red : FColor::Green, false, 0.5f);
	}
	return !bBlocked;
}

bool UFootholdTraceLibrary::TraceDown(const FFootholdQueryContext& Ctx, const FVector& Start, float Distance, FHitResult& OutHit)
{
	if (!Ctx.World)
	{
		return false;
	}
	const FVector End = Start - Ctx.Up * Distance;
	const bool bHit = Ctx.World->LineTraceSingleByChannel(OutHit, Start, End, Ctx.GetTraceChannel(), Ctx.MakeQueryParams(TEXT("FootholdDown")));
	if (UE::Foothold::Trace::ShouldDraw(Ctx))
	{
		DrawDebugLine(Ctx.World, Start, bHit ? OutHit.ImpactPoint : End, bHit ? FColor::Cyan : FColor::Silver, false, 0.5f);
	}
	return bHit && !OutHit.bStartPenetrating;
}

FFootholdWallHit UFootholdTraceLibrary::FindWall(const FFootholdQueryContext& Ctx, const FVector& Direction, float Distance, float HeightOffset)
{
	using namespace UE::Foothold::Trace;

	FFootholdWallHit Result;
	const FVector Dir = Planar(Direction, Ctx.Up);
	if (!Ctx.World || !Ctx.Settings || Dir.IsNearlyZero())
	{
		return Result;
	}

	// A small sphere from the capsule centre line out past the capsule surface: it reaches the same
	// walls the capsule would touch, without catching the floor.
	const float ProbeRadius = Ctx.CapsuleRadius * 0.4f;
	const FVector Start = Ctx.Location + Ctx.Up * HeightOffset;
	const FVector End = Start + Dir * (Ctx.CapsuleRadius + Distance - ProbeRadius);

	FHitResult Hit;
	const FCollisionQueryParams Params = Ctx.MakeQueryParams(TEXT("FootholdWall"));
	bool bHit = Ctx.World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, Ctx.GetTraceChannel(), FCollisionShape::MakeSphere(ProbeRadius), Params);
	if (bHit && Hit.bStartPenetrating)
	{
		// Already touching: a line trace still gives a usable point and normal.
		bHit = Ctx.World->LineTraceSingleByChannel(Hit, Start, End + Dir * ProbeRadius, Ctx.GetTraceChannel(), Params) && !Hit.bStartPenetrating;
	}

	if (ShouldDraw(Ctx))
	{
		DrawDebugLine(Ctx.World, Start, bHit ? Hit.ImpactPoint : End, bHit ? FColor::Orange : FColor::Silver, false, 0.5f);
	}

	if (!bHit)
	{
		return Result;
	}

	const FVector Normal = Hit.ImpactNormal;
	if (FMath::Abs(FVector::DotProduct(Normal, Ctx.Up)) > Ctx.Settings->GetWallUpDotMax() || FVector::DotProduct(Normal, Dir) > 0.f)
	{
		return Result;	// a floor, a ceiling or a back face: not a wall
	}

	Result.bHit = true;
	Result.Location = Hit.ImpactPoint;
	Result.Normal = Normal;
	Result.FlatNormal = Planar(Normal, Ctx.Up);
	Result.Distance = FMath::Max(0.f, FVector::DotProduct(Hit.ImpactPoint - Start, Dir) - Ctx.CapsuleRadius);
	Result.Component = Hit.GetComponent();
	Result.bClimbable = IsClimbable(Hit, *Ctx.Settings, Ctx.Up);
	Result.bWallRunnable = !HasTag(Hit, Ctx.Settings->NoWallRunTag) && FMath::Abs(FVector::DotProduct(Normal, Ctx.Up)) < 0.2f;
	return Result;
}

FFootholdLedge UFootholdTraceLibrary::FindLedge(const FFootholdQueryContext& Ctx, const FVector& Direction, float MinHeightAboveFeet, float MaxHeightAboveFeet, float Reach)
{
	using namespace UE::Foothold::Trace;

	FFootholdLedge Result;
	const FVector Dir = Planar(Direction, Ctx.Up);
	if (!Ctx.World || !Ctx.Settings || Dir.IsNearlyZero() || MaxHeightAboveFeet <= MinHeightAboveFeet)
	{
		return Result;
	}
	const UFootholdSharedSettings& Settings = *Ctx.Settings;
	const FVector Feet = Ctx.GetFeet();

	// 1. Find the wall. Probe at a few heights between just above the floor and just above the
	//    highest edge, and keep the nearest wall: low obstacles, chest-high walls and ledges above
	//    the head all show up at one of them.
	FFootholdWallHit Wall;
	{
		const float Lowest = FMath::Max(LowestWallProbe, MinHeightAboveFeet - 10.f);
		const float Highest = MaxHeightAboveFeet - 5.f;
		const int32 NumProbes = 5;
		for (int32 Index = 0; Index < NumProbes; ++Index)
		{
			const float Height = FMath::Lerp(Lowest, Highest, static_cast<float>(Index) / (NumProbes - 1));
			if (Height < Lowest - KINDA_SMALL_NUMBER)
			{
				continue;
			}
			const FFootholdWallHit Probe = FindWall(Ctx, Dir, Reach, Height - Ctx.CapsuleHalfHeight);
			if (Probe.bHit && (!Wall.bHit || Probe.Distance < Wall.Distance - 1.f))
			{
				Wall = Probe;
			}
		}
	}
	if (!Wall.bHit)
	{
		return Result;
	}

	// 2. Find the top: trace down just behind the wall face, from above the highest allowed edge.
	const FVector WallFlatNormal = Wall.FlatNormal;
	const FVector Into = -WallFlatNormal;
	const FVector WallFaceAtFeet = Wall.Location - Ctx.Up * FVector::DotProduct(Wall.Location - Feet, Ctx.Up);
	const FVector ProbeColumn = WallFaceAtFeet + Into * (EdgeInset + Settings.MinLedgeDepth * 0.5f);

	// Start in free space as high as allowed. A start inside geometry means either the wall continues
	// above the highest allowed edge, or there's a ceiling over the ledge: step down until the start is
	// free. If it never is, the wall is too tall.
	FVector TopProbeStart = FVector::ZeroVector;
	bool bFoundFreeStart = false;
	for (float StartHeight = MaxHeightAboveFeet + TopProbeMargin; StartHeight > MinHeightAboveFeet; StartHeight -= TopStartStep)
	{
		const FVector Candidate = ProbeColumn + Ctx.Up * StartHeight;
		if (!Ctx.World->OverlapBlockingTestByChannel(Candidate, FQuat::Identity, Ctx.GetTraceChannel(), FCollisionShape::MakeSphere(2.f), Ctx.MakeQueryParams(TEXT("FootholdTopStart"))))
		{
			TopProbeStart = Candidate;
			bFoundFreeStart = true;
			break;
		}
	}
	if (!bFoundFreeStart)
	{
		return Result;
	}

	FHitResult TopHit;
	const float StartHeightAboveFeet = FVector::DotProduct(TopProbeStart - Feet, Ctx.Up);
	if (!TraceDown(Ctx, TopProbeStart, StartHeightAboveFeet - MinHeightAboveFeet + 5.f, TopHit))
	{
		return Result;
	}
	const float TopHeight = FVector::DotProduct(TopHit.ImpactPoint - Feet, Ctx.Up);
	if (TopHeight < MinHeightAboveFeet || TopHeight > MaxHeightAboveFeet)
	{
		return Result;
	}
	if (HasTag(TopHit, Settings.NoClimbTag) || (Wall.Component.IsValid() && Wall.Component->ComponentHasTag(Settings.NoClimbTag))
		|| (Wall.Component.IsValid() && Wall.Component->GetOwner() && Wall.Component->GetOwner()->ActorHasTag(Settings.NoClimbTag)))
	{
		return Result;
	}
	const bool bTopWalkable = IsWalkable(TopHit.ImpactNormal, Ctx.Up, Settings.MaxWalkSlopeCosine);
	if (!bTopWalkable && !Settings.bAllowHangOnUnstandableLedges)
	{
		return Result;
	}

	// 3. Read the wall again just under the top, for the edge position and its normal there.
	const FVector UnderEdgeStart = Ctx.Location + Ctx.Up * (FVector::DotProduct(TopHit.ImpactPoint - Ctx.Location, Ctx.Up) - UnderEdgeProbe);
	FHitResult EdgeWallHit;
	const FVector EdgeWallEnd = UnderEdgeStart + Dir * (Ctx.CapsuleRadius + Reach + EdgeInset + Settings.MinLedgeDepth);
	FVector EdgeNormal = WallFlatNormal;
	FVector EdgeWallPoint = WallFaceAtFeet + Ctx.Up * FVector::DotProduct(TopHit.ImpactPoint - WallFaceAtFeet, Ctx.Up);
	if (Ctx.World->LineTraceSingleByChannel(EdgeWallHit, UnderEdgeStart, EdgeWallEnd, Ctx.GetTraceChannel(), Ctx.MakeQueryParams(TEXT("FootholdEdge"))) && !EdgeWallHit.bStartPenetrating)
	{
		const FVector Flat = Planar(EdgeWallHit.ImpactNormal, Ctx.Up);
		if (!Flat.IsNearlyZero() && FMath::Abs(FVector::DotProduct(EdgeWallHit.ImpactNormal, Ctx.Up)) <= Settings.GetWallUpDotMax())
		{
			EdgeNormal = Flat;
			EdgeWallPoint = EdgeWallHit.ImpactPoint;
		}
	}
	const FVector EdgePoint = EdgeWallPoint + Ctx.Up * FVector::DotProduct(TopHit.ImpactPoint - EdgeWallPoint, Ctx.Up);

	// 4. The top must run back at least MinLedgeDepth from the edge. On a sloped top the height there
	//    legitimately differs from the edge's, by up to depth * tan(slope).
	FHitResult DepthHit;
	const float DepthDistance = Settings.MinLedgeDepth + EdgeInset;
	const float TopCos = FMath::Clamp(FVector::DotProduct(TopHit.ImpactNormal, Ctx.Up), 0.05f, 1.f);
	const float SlopeAllowance = DepthDistance * FMath::Sqrt(1.f - TopCos * TopCos) / TopCos;
	const float DepthTolerance = 15.f + SlopeAllowance;
	const FVector DepthStart = EdgePoint - EdgeNormal * DepthDistance + Ctx.Up * (DepthTolerance + 5.f);
	if (!TraceDown(Ctx, DepthStart, DepthTolerance * 2.f + 10.f, DepthHit)
		|| FMath::Abs(FVector::DotProduct(DepthHit.ImpactPoint - EdgePoint, Ctx.Up)) > DepthTolerance)
	{
		return Result;
	}

	Result.bValid = true;
	Result.EdgePoint = EdgePoint;
	Result.WallNormal = EdgeNormal;
	Result.TopNormal = TopHit.ImpactNormal;
	Result.EdgeDirection = FVector::CrossProduct(Ctx.Up, EdgeNormal).GetSafeNormal();
	Result.HeightAboveFeet = FVector::DotProduct(EdgePoint - Feet, Ctx.Up);
	Result.Component = TopHit.GetComponent();

	// 5. Hang and stand positions, and whether the capsule fits at each.
	Result.HangLocation = EdgePoint + EdgeNormal * (Ctx.CapsuleRadius + Settings.WallGap) - Ctx.Up * Settings.HandReachAboveCenter;
	Result.bHangClear = IsCapsuleClear(Ctx, Result.HangLocation);

	FHitResult StandFloor;
	const FVector StandProbe = EdgePoint - EdgeNormal * (Ctx.CapsuleRadius + StandInset) + Ctx.Up * TopProbeMargin;
	if (TraceDown(Ctx, StandProbe, TopProbeMargin * 2.f, StandFloor) && IsWalkable(StandFloor.ImpactNormal, Ctx.Up, Settings.MaxWalkSlopeCosine))
	{
		Result.StandLocation = StandFloor.ImpactPoint + Ctx.Up * (Ctx.CapsuleHalfHeight + 2.f);
		Result.bCanStand = IsCapsuleClear(Ctx, Result.StandLocation);
	}

	if (ShouldDraw(Ctx))
	{
		DrawDebugSphere(Ctx.World, EdgePoint, 6.f, 8, Result.bCanStand ? FColor::Green : FColor::Yellow, false, 0.5f);
		DrawDebugDirectionalArrow(Ctx.World, EdgePoint, EdgePoint + EdgeNormal * 30.f, 8.f, FColor::Blue, false, 0.5f);
	}
	return Result;
}

UFootholdLadderComponent* UFootholdTraceLibrary::FindLadder(const FFootholdQueryContext& Ctx, float MaxDistance, float& OutLadderPosition)
{
	OutLadderPosition = 0.f;
	if (!Ctx.World)
	{
		return nullptr;
	}
	const UFootholdWorldSubsystem* Subsystem = Ctx.World->GetSubsystem<UFootholdWorldSubsystem>();
	if (!Subsystem)
	{
		return nullptr;
	}

	UFootholdLadderComponent* Best = nullptr;
	float BestDistance = MaxDistance;
	for (const TWeakObjectPtr<UFootholdLadderComponent>& Entry : Subsystem->GetLadders())
	{
		UFootholdLadderComponent* Ladder = Entry.Get();
		if (!Ladder)
		{
			continue;
		}

		// Only from the front of the ladder.
		if (FVector::DotProduct(Ctx.Location - Ladder->GetBottom(), Ladder->GetFacing()) <= 0.f)
		{
			continue;
		}

		// The feet must be within the ladder's span (with a little slack at both ends).
		const float FeetAlong = FVector::DotProduct(Ctx.GetFeet() - Ladder->GetBottom(), Ladder->GetAxis());
		if (FeetAlong < -30.f || FeetAlong > Ladder->Height + 30.f)
		{
			continue;
		}

		const float Position = Ladder->ProjectToLadder(Ctx.Location);
		const FVector ClimbLocation = Ladder->GetClimbLocation(Position);
		const FVector Offset = Ctx.Location - ClimbLocation;
		const float Distance = FVector::VectorPlaneProject(Offset, Ladder->GetAxis()).Size();
		if (Distance <= BestDistance)
		{
			Best = Ladder;
			BestDistance = Distance;
			OutLadderPosition = Position;
		}
	}
	return Best;
}

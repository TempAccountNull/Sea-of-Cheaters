#pragma once
#include <Windows.h>
#include <UE4/UE4.h>
#include <string>

#ifdef _MSC_VER
#pragma pack(push, 0x8)
#endif


struct FNameEntry
{
	uint32_t Index;
	uint32_t pad;
	FNameEntry* HashNext;
	char AnsiName[1024];

	const int GetIndex() const { return Index >> 1; }
	const char* GetAnsiName() const { return AnsiName; }
};

class TNameEntryArray
{
public:

	bool IsValidIndex(uint32_t index) const { return index < NumElements; }

	FNameEntry const* GetById(uint32_t index) const { return *GetItemPtr(index); }

	FNameEntry const* const* GetItemPtr(uint32_t Index) const {
		const auto ChunkIndex = Index / 16384;
		const auto WithinChunkIndex = Index % 16384;
		const auto Chunk = Chunks[ChunkIndex];
		return Chunk + WithinChunkIndex;
	}

	FNameEntry** Chunks[128];
	uint32_t NumElements = 0;
	uint32_t NumChunks = 0;
};

struct FName
{
	int ComparisonIndex = 0;
	int Number = 0;

	static inline TNameEntryArray* GNames = nullptr;

	static const char* GetNameByIdFast(int Id) {
		auto NameEntry = GNames->GetById(Id);
		if (!NameEntry) return nullptr;
		return NameEntry->GetAnsiName();
	}

	static std::string GetNameById(int Id) {
		auto NameEntry = GNames->GetById(Id);
		if (!NameEntry) return std::string();
		return NameEntry->GetAnsiName();
	}

	const char* GetNameFast() const {
		auto NameEntry = GNames->GetById(ComparisonIndex);
		if (!NameEntry) return nullptr;
		return NameEntry->GetAnsiName();
	}

	const std::string GetName() const {
		auto NameEntry = GNames->GetById(ComparisonIndex);
		if (!NameEntry) return std::string();
		return NameEntry->GetAnsiName();
	};

	inline bool operator==(const FName& other) const {
		return ComparisonIndex == other.ComparisonIndex;
	};

	FName() {}

	FName(const char* find) {
		for (auto i = 6000u; i < GNames->NumElements; i++)
		{
			auto name = GetNameByIdFast(i);
			if (!name) continue;
			if (strcmp(name, find) == 0) {
				ComparisonIndex = i;
				return;
			};
		}
	}
};

struct FUObjectItem
{
	class UObject* Object;
	int Flags;
	int ClusterIndex;
	int SerialNumber;
	int pad;
};

struct TUObjectArray
{
	FUObjectItem* Objects;
	int MaxElements;
	int NumElements;

	class UObject* GetByIndex(int index) { return Objects[index].Object; }
};

class UClass;
class UObject
{
public:
	UObject(UObject* addr) { *this = addr; }
	static inline TUObjectArray* GObjects = nullptr;
	void* Vtable; // 0x0
	int ObjectFlags; // 0x8
	int InternalIndex; // 0xC
	UClass* Class; // 0x10
	FName Name; // 0x18
	UObject* Outer; // 0x20

	std::string GetName() const;

	const char* GetNameFast() const;

	std::string GetFullName() const;

	template<typename T>
	static T* FindObject(const std::string& name)
	{
		for (int i = 0; i < GObjects->NumElements; ++i)
		{
			auto object = GObjects->GetByIndex(i);

			if (object == nullptr)
			{
				continue;
			}

			if (object->GetFullName() == name)
			{
				return static_cast<T*>(object);
			}
		}
		return nullptr;
	}

	static UClass* FindClass(const std::string& name)
	{
		return FindObject<UClass>(name);
	}

	template<typename T>
	static T* GetObjectCasted(uint32_t index)
	{
		return static_cast<T*>(GObjects->GetByIndex(index));
	}

	bool IsA(UClass* cmp) const;

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindObject<UClass>("Class CoreUObject.Object");
		return ptr;
	}
};

class UField : public UObject
{
public:
	using UObject::UObject;
	UField* Next;
};

class UProperty : public UField
{
public:
	int ArrayDim;
	int ElementSize;
	uint64_t PropertyFlags;
	char pad[0xC];
	int Offset_Internal;
	UProperty* PropertyLinkNext;
	UProperty* NextRef;
	UProperty* DestructorLinkNext;
	UProperty* PostConstructLinkNext;
};


class UStruct : public UField
{
public:
	using UField::UField;

	UStruct* SuperField;
	UField* Children;
	int PropertySize;
	int MinAlignment;
	TArray<uint8_t> Script;
	UProperty* PropertyLink;
	UProperty* RefLink;
	UProperty* DestructorLink;
	UProperty* PostConstructLink;
	TArray<UObject*> ScriptObjectReferences;
};

class UFunction : public UStruct
{
public:
	int FunctionFlags;
	uint16_t RepOffset;
	uint8_t NumParms;
	char pad;
	uint16_t ParmsSize;
	uint16_t ReturnValueOffset;
	uint16_t RPCId;
	uint16_t RPCResponseId;
	UProperty* FirstPropertyToInit;
	UFunction* EventGraphFunction; //0x00A0
	int EventGraphCallOffset;
	char pad_0x00AC[0x4]; //0x00AC
	void* Func; //0x00B0
};


inline void ProcessEvent(void* obj, UFunction* function, void* parms)
{
	auto vtable = *reinterpret_cast<void***>(obj);
	reinterpret_cast<void(*)(void*, UFunction*, void*)>(vtable[59])(obj, function, parms);
}

class UClass : public UStruct
{
public:
	using UStruct::UStruct;
	unsigned char                                      UnknownData00[0x138];                                     // 0x0088(0x0138) MISSED OFFSET

	template<typename T>
	inline T* CreateDefaultObject()
	{
		return static_cast<T*>(CreateDefaultObject());
	}


};

class FString : public TArray<wchar_t>
{
public:
	inline FString()
	{
	};

	FString(const wchar_t* other)
	{
		Max = Count = *other ? static_cast<int>(std::wcslen(other)) + 1 : 0;

		if (Count)
		{
			Data = const_cast<wchar_t*>(other);
		}
	};
	FString(const wchar_t* other, int count)
	{
		Data = const_cast<wchar_t*>(other);;
		Max = Count = count;
	};

	inline bool IsValid() const
	{
		return Data != nullptr;
	}

	inline const wchar_t* wide() const
	{
		return Data;
	}

	int multi(char* name, int size) const
	{
		return WideCharToMultiByte(CP_UTF8, 0, Data, Count, name, size, nullptr, nullptr) - 1;
	}
};




enum class EPlayerActivityType : uint8_t
{
	None = 0,
	Bailing = 1,
	Cannon = 2,
	Cannon_END = 3,
	Capstan = 4,
	Capstan_END = 5,
	CarryingBooty = 6,
	CarryingBooty_END = 7,
	Dead = 8,
	Dead_END = 9,
	Digging = 10,
	Dousing = 11,
	EmptyingBucket = 12,
	Harpoon = 13,
	Harpoon_END = 14,
	LoseHealth = 15,
	Repairing = 16,
	Sails = 17,
	Sails_END = 18,
	Wheel = 19,
	Wheel_END = 20
};

struct FPirateDescription
{

};

struct APlayerState {
	char pad[0x03D8];
	FString PlayerName; // 0x03D8

	EPlayerActivityType GetPlayerActivity()
	{
		static auto fn = UObject::FindObject<UFunction>("Function Athena.AthenaPlayerState.GetPlayerActivity");
		EPlayerActivityType activity;
		ProcessEvent(this, fn, &activity);
		return activity;
	}


	FPirateDescription GetPirateDesc()
	{
		static auto fn = UObject::FindObject<UFunction>("Function Athena.AthenaPlayerState.GetPirateDesc");
		FPirateDescription desc;
		ProcessEvent(this, fn, &desc);
		return desc;
	}

};

struct FMinimalViewInfo {
	FVector Location;
	FRotator Rotation;
	char UnknownData00[0x10];
	float FOV;
};

struct FCameraCacheEntry {
	float TimeStamp;
	char pad[0x0010];
	FMinimalViewInfo POV;
};

struct FTViewTarget
{
	class AActor* Target;                                                    // 0x0000(0x0008) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	unsigned char                                      UnknownData_KHBN[0x8];                                     // 0x0008(0x0008) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	struct FMinimalViewInfo                            POV;                                                       // 0x0010(0x05A0) (Edit, BlueprintVisible)
	class APlayerState* PlayerState;                                               // 0x05B0(0x0008) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, Protected, HasGetValueTypeHash)
	unsigned char                                      UnknownData_QIMB[0x8];                                     // 0x05B8(0x0008) MISSED OFFSET (PADDING)

};

struct APlayerCameraManager {
	char pad[0x0440];
	FCameraCacheEntry CameraCache; // 0x0440
	FCameraCacheEntry LastFrameCameraCache;
	FTViewTarget ViewTarget;
	FTViewTarget PendingViewTarget;


	FVector GetCameraLocation() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerCameraManager.GetCameraLocation");
		FVector location;
		ProcessEvent(this, fn, &location);
		return location;
	};
	FRotator GetCameraRotation() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerCameraManager.GetCameraRotation");
		FRotator rotation;
		ProcessEvent(this, fn, &rotation);
		return rotation;
	}
};


struct FKey
{
	FName KeyName;
	unsigned char UnknownData00[0x18] = {};

	FKey() {};
	FKey(const char* InName) : KeyName(FName(InName)) {}
};

struct AController {

	char pad_0000[0x03E8];
	class ACharacter* Character; //0x03E8
	char pad_0480[0x70];
	APlayerCameraManager* PlayerCameraManager; //0x0460
	char pad_04f8[0x1031];
	bool IdleDisconnectEnabled; // 0x1499

	void SendToConsole(FString& cmd) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerController.SendToConsole");
		ProcessEvent(this, fn, &cmd);
	}

	bool WasInputKeyJustPressed(const FKey& Key) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerController.WasInputKeyJustPressed");
		struct
		{
			FKey Key;

			bool ReturnValue = false;
		} params;

		params.Key = Key;
		ProcessEvent(this, fn, &params);

		return params.ReturnValue;
	}

	bool ProjectWorldLocationToScreen(const FVector& WorldLocation, FVector2D& ScreenLocation) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerController.ProjectWorldLocationToScreen");
		struct
		{
			FVector WorldLocation;
			FVector2D ScreenLocation;
			bool ReturnValue = false;
		} params;

		params.WorldLocation = WorldLocation;
		ProcessEvent(this, fn, &params);
		ScreenLocation = params.ScreenLocation;
		return params.ReturnValue;
	}

	FRotator GetControlRotation() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Pawn.GetControlRotation");
		struct FRotator rotation;
		ProcessEvent(this, fn, &rotation);
		return rotation;
	}

	FRotator GetDesiredRotation() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Pawn.GetDesiredRotation");
		struct FRotator rotation;
		ProcessEvent(this, fn, &rotation);
		return rotation;
	}

	void AddYawInput(float Val) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerController.AddYawInput");
		ProcessEvent(this, fn, &Val);
	}

	void AddPitchInput(float Val) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerController.AddPitchInput");
		ProcessEvent(this, fn, &Val);
	}

	void FOV(float NewFOV) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerController.FOV");
		ProcessEvent(this, fn, &NewFOV);
	}

	bool LineOfSightTo(ACharacter* Other, const FVector& ViewPoint, const bool bAlternateChecks) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Controller.LineOfSightTo");
		struct {
			ACharacter* Other = nullptr;
			FVector ViewPoint;
			bool bAlternateChecks = false;
			bool ReturnValue = false;
		} params;
		params.Other = Other;
		params.ViewPoint = ViewPoint;
		params.bAlternateChecks = bAlternateChecks;
		ProcessEvent(this, fn, &params);
		return params.ReturnValue;
	}
};

struct UHealthComponent {
	float GetMaxHealth() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.HealthComponent.GetMaxHealth");
		float health = 0.f;
		ProcessEvent(this, fn, &health);
		return health;
	}
	float GetCurrentHealth() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.HealthComponent.GetCurrentHealth");
		float health = 0.f;
		ProcessEvent(this, fn, &health);
		return health;
	};
};

struct USkeletalMeshComponent {
	char pad[0x5A8];
	TArray<FTransform> SpaceBasesArray[2];
	int CurrentEditableSpaceBases;
	int CurrentReadSpaceBases;

	FName GetBoneName(int BoneIndex)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.SkinnedMeshComponent.GetBoneName");
		struct
		{
			int BoneIndex = 0;
			FName ReturnValue;
		} params;
		params.BoneIndex = BoneIndex;
		ProcessEvent(this, fn, &params);
		return params.ReturnValue;
	}

	FTransform K2_GetComponentToWorld() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.SceneComponent.K2_GetComponentToWorld");
		FTransform CompToWorld;
		ProcessEvent(this, fn, &CompToWorld);
		return CompToWorld;
	}

	bool GetBone(const uint32_t id, const FMatrix& componentToWorld, FVector& pos) {
		try {
			auto bones = SpaceBasesArray[CurrentReadSpaceBases];
			
			if (id >= bones.Count) return false;
			const auto& bone = bones[id];
			auto boneMatrix = bone.ToMatrixWithScale();
			auto world = boneMatrix * componentToWorld;
			pos = { world.M[3][0], world.M[3][1], world.M[3][2] };
			return true;
		}
		catch (...)
		{
			printf("error %d\n", __LINE__);
			return false;
		}
	}
};

struct AShipInternalWater {
	float GetNormalizedWaterAmount() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.ShipInternalWater.GetNormalizedWaterAmount");
		float params = 0.f;
		ProcessEvent(this, fn, &params);
		return params;
	}
};

struct AHullDamage {
	char pad[0x0418];
	TArray<class ACharacter*> ActiveHullDamageZones; // 0x0418
};

struct UDrowningComponent {
	float GetOxygenLevel() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.DrowningComponent.GetOxygenLevel");
		float oxygen;
		ProcessEvent(this, fn, &oxygen);
		return oxygen;
	}
};

struct AFauna {
	char pad1[0x0808];
	FString* DisplayName; // 0x0808
};

enum class ESwimmingCreatureType : uint8_t
{
	SwimmingCreature = 0,
	Shark = 1,
	TinyShark = 2,
	Siren = 3,
	ESwimmingCreatureType_MAX = 4
};

struct ASharkPawn {
	char pad1[0x04C8];
	USkeletalMeshComponent* Mesh; // 0x04C8
	char pad2[0x5C];
	ESwimmingCreatureType SwimmingCreatureType; // 0x052C
};

struct FAIEncounterSpecification
{
	char pad[0x80];
	FString* LocalisableName; // 0x0080
};

struct UItemDesc {
	char pad[0x0028];
	FString* Title; // 0x0028(0x0038)
};

struct AItemInfo {
	char pad[0x0430];
	UItemDesc* Desc; // 0x0430
};

struct UWieldedItemComponent {
	char pad[0x02C0];
	ACharacter* CurrentlyWieldedItem; // 0x02C0
};

struct FWeaponProjectileParams
{
	float                                              Damage;                                                   // 0x0000(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              DamageMultiplierAtMaximumRange;                           // 0x0004(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              LifeTime;                                                 // 0x0008(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              TrailFadeOutTime;                                         // 0x000C(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              Velocity;                                                 // 0x0010(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	char pad_78[0x94];
};

struct FProjectileShotParams
{
	int                                                Seed;                                                     // 0x0000(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              ProjectileDistributionMaxAngle;                           // 0x0004(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	int                                                NumberOfProjectiles;                                      // 0x0008(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              ProjectileMaximumRange;                                   // 0x000C(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              ProjectileDamage;                                         // 0x0010(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              ProjectileDamageMultiplierAtMaximumRange;                 // 0x0014(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
};

struct FProjectileWeaponParameters {
	int                                                AmmoClipSize;                                             // 0x0000(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	int                                                AmmoCostPerShot;                                          // 0x0004(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              EquipDuration;                                            // 0x0008(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              IntoAimingDuration;                                       // 0x000C(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              RecoilDuration;                                           // 0x0010(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              ReloadDuration;                                           // 0x0014(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	struct FProjectileShotParams                       HipFireProjectileShotParams;                              // 0x0018(0x0018) (Edit, BlueprintVisible)
	struct FProjectileShotParams                       AimDownSightsProjectileShotParams;                        // 0x0030(0x0018) (Edit, BlueprintVisible)
	int                                                Seed;                                                     // 0x0048(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              ProjectileDistributionMaxAngle;                           // 0x004C(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	int                                                NumberOfProjectiles;                                      // 0x0050(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              ProjectileMaximumRange;                                   // 0x0054(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              ProjectileDamage;                                         // 0x0058(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              ProjectileDamageMultiplierAtMaximumRange;                 // 0x005C(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	class UClass* DamagerType;                                              // 0x0060(0x0008) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	class UClass* ProjectileId;                                             // 0x0068(0x0008) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	struct FWeaponProjectileParams                     AmmoParams;                                               // 0x0070(0x00A8) (Edit, BlueprintVisible)
	bool                                               UsesScope;                                                // 0x0118(0x0001) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	unsigned char                                      UnknownData00[0x3];                                       // 0x0119(0x0003) MISSED OFFSET
	float                                              ZoomedRecoilDurationIncrease;                             // 0x011C(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              SecondsUntilZoomStarts;                                   // 0x0120(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              SecondsUntilPostStarts;                                   // 0x0124(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              WeaponFiredAINoiseRange;                                  // 0x0128(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              MaximumRequestPositionDelta;                              // 0x012C(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              MaximumRequestAngleDelta;                                 // 0x0130(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              TimeoutTolerance;                                         // 0x0134(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              AimingMoveSpeedScalar;                                    // 0x0138(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	char pad_342[0x4];
	float                                              InAimFOV;                                                 // 0x0140(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData)
	float                                              BlendSpeed;                                               // 0x0144(0x0004) (Edit, BlueprintVisible, 
};

struct AProjectileWeapon {
	char pad[0x07c0]; // 0
	FProjectileWeaponParameters WeaponParameters; // 0x07c0

	bool CanFire()
	{
		static auto fn = UObject::FindObject<UFunction>("Function Athena.ProjectileWeapon.CanFire");
		bool canfire;
		ProcessEvent(this, fn, &canfire);
		return canfire;
	}


};


struct FHitResult
{
	unsigned char                                      bBlockingHit : 1;                                         // 0x0000(0x0001)
	unsigned char                                      bStartPenetrating : 1;                                    // 0x0000(0x0001)
	unsigned char                                      UnknownData00[0x3];                                       // 0x0001(0x0003) MISSED OFFSET
	float                                              Time;                                                     // 0x0004(0x0004) (ZeroConstructor, IsPlainOldData)
	float                                              Distance;                                                 // 0x0008(0x0004) (ZeroConstructor, IsPlainOldData)
	char pad_5894[0x48];
	float                                              PenetrationDepth;                                         // 0x0054(0x0004) (ZeroConstructor, IsPlainOldData)
	int                                                Item;                                                     // 0x0058(0x0004) (ZeroConstructor, IsPlainOldData)
	char pad_3424[0x18];
	struct FName                                       BoneName;                                                 // 0x0074(0x0008) (ZeroConstructor, IsPlainOldData)
	int                                                FaceIndex;                                                // 0x007C(0x0004) (ZeroConstructor, IsPlainOldData)
};

struct UWorldMapIslandDataAsset {
	char pad[0x48];
	FVector WorldSpaceCameraPosition; // 0x0018
	// ADD THE OFFSET OF CAPTUREPARAMS TO THIS OFFSET
};

struct UIslandDataAssetEntry {
	char pad[0x0040];
	UWorldMapIslandDataAsset* WorldMapData; // 0x0040
	char pad2[0x0068];
	FString* LocalisedName; // 0x00B0
};

struct UIslandDataAsset {
	char pad[0x0048];
	TArray<UIslandDataAssetEntry*> IslandDataEntries; // 0x0048
};

struct AIslandService {
	char pad[0x0460];
	UIslandDataAsset* IslandDataAsset; // 0x460
};

struct ASlidingDoor {
	char pad_0x0[0x52C];
	FVector InitialDoorMeshLocation; // 0x052C

	void OpenDoor() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.SkeletonFortDoor.OpenDoor");
		ProcessEvent(this, fn, nullptr);
	}
};


enum class EDrawDebugTrace : uint8_t
{
	EDrawDebugTrace__None = 0,
	EDrawDebugTrace__ForOneFrame = 1,
	EDrawDebugTrace__ForDuration = 2,
	EDrawDebugTrace__Persistent = 3,
	EDrawDebugTrace__EDrawDebugTrace_MAX = 4
};

// Enum Engine.ETraceTypeQuery
enum class ETraceTypeQuery : uint8_t
{
	TraceTypeQuery1 = 0,
	TraceTypeQuery2 = 1,
	TraceTypeQuery3 = 2,
	TraceTypeQuery4 = 3,
	TraceTypeQuery5 = 4,
	TraceTypeQuery6 = 5,
	TraceTypeQuery7 = 6,
	TraceTypeQuery8 = 7,
	TraceTypeQuery9 = 8,
	TraceTypeQuery10 = 9,
	TraceTypeQuery11 = 10,
	TraceTypeQuery12 = 11,
	TraceTypeQuery13 = 12,
	TraceTypeQuery14 = 13,
	TraceTypeQuery15 = 14,
	TraceTypeQuery16 = 15,
	TraceTypeQuery17 = 16,
	TraceTypeQuery18 = 17,
	TraceTypeQuery19 = 18,
	TraceTypeQuery20 = 19,
	TraceTypeQuery21 = 20,
	TraceTypeQuery22 = 21,
	TraceTypeQuery23 = 22,
	TraceTypeQuery24 = 23,
	TraceTypeQuery25 = 24,
	TraceTypeQuery26 = 25,
	TraceTypeQuery27 = 26,
	TraceTypeQuery28 = 27,
	TraceTypeQuery29 = 28,
	TraceTypeQuery30 = 29,
	TraceTypeQuery31 = 30,
	TraceTypeQuery32 = 31,
	TraceTypeQuery_MAX = 32,
	ETraceTypeQuery_MAX = 33
};

struct USceneComponent {
	FVector K2_GetComponentLocation() {
		FVector location;
		static auto fn = UObject::FindObject<UFunction>("Function Engine.SceneComponent.K2_GetComponentLocation");
		ProcessEvent(this, fn, &location);
		return location;
	}
};

struct APuzzleVault {
	char pad[0x1000];
	ASlidingDoor* OuterDoor; // 0x1000
};

struct FGuid
{
	int                                                A;                                                         // 0x0000(0x0004) (Edit, ZeroConstructor, SaveGame, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	int                                                B;                                                         // 0x0004(0x0004) (Edit, ZeroConstructor, SaveGame, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	int                                                C;                                                         // 0x0008(0x0004) (Edit, ZeroConstructor, SaveGame, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	int                                                D;                                                         // 0x000C(0x0004) (Edit, ZeroConstructor, SaveGame, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
};

struct FSessionTemplate
{
	struct FString                                     TemplateName;                                              // 0x0000(0x0010) (ZeroConstructor, Protected, HasGetValueTypeHash)
	unsigned char             SessionType;                                               // 0x0010(0x0001) (ZeroConstructor, IsPlainOldData, NoDestructor, Protected, HasGetValueTypeHash)
	unsigned char                                      UnknownData_2Q1C[0x3];                                     // 0x0011(0x0003) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	int                                                MaxPlayers;                                                // 0x0014(0x0004) (ZeroConstructor, IsPlainOldData, NoDestructor, Protected, HasGetValueTypeHash)

};

// ScriptStruct Sessions.CrewSessionTemplate
// 0x0020 (0x0038 - 0x0018)
struct FCrewSessionTemplate : public FSessionTemplate
{
	struct FString                                     MatchmakingHopper;                                         // 0x0018(0x0010) (ZeroConstructor, HasGetValueTypeHash)
	class UClass* ShipSize;                                                  // 0x0028(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, UObjectWrapper, HasGetValueTypeHash)
	int                                                MaxMatchmakingPlayers;                                     // 0x0030(0x0004) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	unsigned char                                      UnknownData_JPXK[0x4];                                     // 0x0034(0x0004) MISSED OFFSET (PADDING)

};

struct FCrew
{
	struct FGuid                                       CrewId;                                                   // 0x0000(0x0010) (ZeroConstructor, IsPlainOldData)
	struct FGuid                                       SessionId;                                                // 0x0010(0x0010) (ZeroConstructor, IsPlainOldData)
	TArray<class APlayerState*>                        Players;                                                  // 0x0020(0x0010) (ZeroConstructor)
	struct FCrewSessionTemplate                        CrewSessionTemplate;                                      // 0x0030(0x0038)
	struct FGuid                                       LiveryID;                                                 // 0x0068(0x0010) (ZeroConstructor, IsPlainOldData)
	unsigned char                                      UnknownData00[0x8];                                       // 0x0078(0x0008) MISSED OFFSET
	TArray<class AActor*>                              AssociatedActors;                                         // 0x0080(0x0010) (ZeroConstructor)
};

struct ACrewService {
	char pad[0x04A8];
	TArray<FCrew> Crews; // 0x04A8
};

struct AShipService
{
	int GetNumShips()
	{
		static auto fn = UObject::FindObject<UFunction>("Function Athena.ShipService.GetNumShips");
		int num;
		ProcessEvent(this, fn, &num);
		return num;
	}
};

struct AKrakenService
{
	char pad[0x0570];
	class AKraken* Kraken; // 0x05B8(0x0008)
	bool IsKrakenActive() {
		static auto fn = UObject::FindObject<UFunction>("Function Kraken.KrakenService.IsKrakenActive");
		bool isActive;
		ProcessEvent(this, fn, &isActive);
		return isActive;
	}

	void RequestKrakenWithLocation(const FVector& SpawnLocation, ACharacter* SpawnedForActor) {
		static auto fn = UObject::FindObject<UFunction>("Function Kraken.KrakenService.RequestKrakenWithLocation");
		struct {
			FVector SpawnLocation;
			ACharacter* SpawnedForActor = nullptr;
		} params;
		params = { SpawnLocation, SpawnedForActor };
		ProcessEvent(this, fn, &params);
	}

	void DismissKraken()
	{
		static auto fn = UObject::FindObject<UFunction>("Function Kraken.KrakenService.DismissKraken");
		ProcessEvent(this, fn, nullptr);
	}
};

struct AAthenaGameState {
	char pad[0x05B8];
	class AWindService* WindService;                                               // 0x05B8(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class APlayerManagerService* PlayerManagerService;                                      // 0x05C0(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class AShipService* ShipService;                                               // 0x05C8(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class AWatercraftService* WatercraftService;                                         // 0x05D0(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class ATimeService* TimeService;                                               // 0x05D8(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class UHealthCustomizationService* HealthService;                                             // 0x05E0(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class AFFTWaterService* WaterService;                                              // 0x05E8(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class AStormService* StormService;                                              // 0x05F0(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class ACrewService* CrewService;                                               // 0x05F8(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class AContestZoneService* ContestZoneService;                                        // 0x0600(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class AContestRowboatsService* ContestRowboatsService;                                    // 0x0608(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class AIslandService* IslandService;                                             // 0x0610(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class ANPCService* NPCService;                                                // 0x0618(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class ASkellyFortService* SkellyFortService;                                         // 0x0620(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class AAIDioramaService* AIDioramaService;                                          // 0x0628(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class AAshenLordEncounterService* AshenLordEncounterService;                                 // 0x0630(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class AAggressiveGhostShipsEncounterService* AggressiveGhostShipsEncounterService;                      // 0x0638(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class ATallTaleService* TallTaleService;                                           // 0x0640(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class AAIShipObstacleService* AIShipObstacleService;                                     // 0x0648(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class AAIShipService* AIShipService;                                             // 0x0650(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class AAITargetService* AITargetService;                                           // 0x0658(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UShipLiveryCatalogueService* ShipLiveryCatalogueService;                                // 0x0660(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class AContestManagerService* ContestManagerService;                                     // 0x0668(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class ADrawDebugService* DrawDebugService;                                          // 0x0670(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class AWorldEventZoneService* WorldEventZoneService;                                     // 0x0678(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWorldResourceRegistry* WorldResourceRegistry;                                     // 0x0680(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class AKrakenService* KrakenService;                                             // 0x0688(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class UPlayerNameService* PlayerNameService;                                         // 0x0690(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class ATinySharkService* TinySharkService;                                          // 0x0698(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class AProjectileService* ProjectileService;                                         // 0x06A0(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class ULaunchableProjectileService* LaunchableProjectileService;                               // 0x06A8(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UServerNotificationsService* ServerNotificationsService;                                // 0x06B0(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class AAIManagerService* AIManagerService;                                          // 0x06B8(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class AAIEncounterService* AIEncounterService;                                        // 0x06C0(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class AAIEncounterGenerationService* AIEncounterGenerationService;                              // 0x06C8(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UEncounterService* EncounterService;                                          // 0x06D0(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UGameEventSchedulerService* GameEventSchedulerService;                                 // 0x06D8(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UHideoutService* HideoutService;                                            // 0x06E0(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UAthenaStreamedLevelService* StreamedLevelService;                                      // 0x06E8(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class ULocationProviderService* LocationProviderService;                                   // 0x06F0(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class AHoleService* HoleService;                                               // 0x06F8(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class APlayerBuriedItemService* PlayerBuriedItemService;                                   // 0x0700(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class ULoadoutService* LoadoutService;                                            // 0x0708(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UOcclusionService* OcclusionService;                                          // 0x0710(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UPetsService* PetsService;                                               // 0x0718(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UAthenaAITeamsService* AthenaAITeamsService;                                      // 0x0720(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class AAllianceService* AllianceService;                                           // 0x0728(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class UMaterialAccessibilityService* MaterialAccessibilityService;                              // 0x0730(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class AReapersMarkService* ReapersMarkService;                                        // 0x0738(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class AEmissaryLevelService* EmissaryLevelService;                                      // 0x0740(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class ACampaignService* CampaignService;                                           // 0x0748(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class AFlamesOfFateSettingsService* FlamesOfFateSettingsService;                               // 0x0750(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class AServiceStatusNotificationsService* ServiceStatusNotificationsService;                         // 0x0758(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class UMigrationService* MigrationService;                                          // 0x0760(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class AShroudBreakerService* ShroudBreakerService;                                      // 0x0768(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UServerUpdateReportingService* ServerUpdateReportingService;                              // 0x0770(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class AGenericMarkerService* GenericMarkerService;                                      // 0x0778(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class AMechanismsService* MechanismsService;                                         // 0x0780(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UMerchantContractsService* MerchantContractsService;                                  // 0x0788(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UShipFactory* ShipFactory;                                               // 0x0790(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class URewindPhysicsService* RewindPhysicsService;                                      // 0x0798(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UNotificationMessagesDataAsset* NotificationMessagesDataAsset;                             // 0x07A0(0x0008) (Edit, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class AProjectileCooldownService* ProjectileCooldownService;                                 // 0x07A8(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UIslandReservationService* IslandReservationService;                                  // 0x07B0(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class APortalService* PortalService;                                             // 0x07B8(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UMeshMemoryConstraintService* MeshMemoryConstraintService;                               // 0x07C0(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class ABootyStorageService* BootyStorageService;                                       // 0x07C8(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class ASpireService* SpireService;                                              // 0x07D0(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class UAirGivingService* AirGivingService;                                          // 0x07D8(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UCutsceneService* CutsceneService;                                           // 0x07E0(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class ACargoRunService* CargoRunService;                                           // 0x07E8(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class ACommodityDemandService* CommodityDemandService;                                    // 0x07F0(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class ADebugTeleportationDestinationService* DebugTeleportationDestinationService;                      // 0x07F8(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	class ASeasonProgressionUIService* SeasonProgressionUIService;                                // 0x0800(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UTunnelsOfTheDamnedService* TunnelsOfTheDamnedService;                                 // 0x0808(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWorldSequenceService* WorldSequenceService;                                      // 0x0810(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UCustomVaultService* CustomVaultService;                                        // 0x0A78(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)

};

struct UCharacterMovementComponent {
	FVector GetCurrentAcceleration() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.CharacterMovementComponent.GetCurrentAcceleration");
		FVector acceleration;
		ProcessEvent(this, fn, &acceleration);
		return acceleration;
	}
};


struct FFloatRange {
	float pad1;
	float min;
	float pad2;
	float max;
};

struct ACannon {
	char pad_4324[0x0528];
	class USkeletalMeshComponent* BaseMeshComponent;                                         // 0x0528(0x0008) (Edit, BlueprintVisible, ExportObject, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, InstancedReference, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UStaticMeshComponent* BarrelMeshComponent;                                       // 0x0530(0x0008) (Edit, BlueprintVisible, ExportObject, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, InstancedReference, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UStaticMeshComponent* FuseMeshComponent;                                         // 0x00538(0x0008) (Edit, BlueprintVisible, ExportObject, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, InstancedReference, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UReplicatedShipPartCustomizationComponent* CustomizationComponent;                                    // 0x0540(0x0008) (Edit, BlueprintVisible, ExportObject, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, InstancedReference, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class ULoadableComponent* LoadableComponent;                                         // 0x0538(0x0008) (Edit, BlueprintVisible, ExportObject, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, InstancedReference, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class ULoadingPointComponent* LoadingPointComponent;                                     // 0x0540(0x0008) (Edit, BlueprintVisible, ExportObject, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, InstancedReference, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UChildActorComponent* CannonBarrelInteractionComponent;                          // 0x0548(0x0008) (Edit, BlueprintVisible, ExportObject, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, InstancedReference, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	struct FName                                       CameraSocket;                                              // 0x0550(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	struct FName                                       CameraInsideCannonSocket;                                  // 0x0558(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	struct FName                                       LaunchSocket;                                              // 0x0560(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	struct FName                                       TooltipSocket;                                             // 0x0568(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	struct FName                                       AudioAimRTPCName;                                          // 0x0570(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	struct FName                                       InsideCannonRTPCName;                                      // 0x0578(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UClass* ProjectileClass;                                           // 0x0580(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, UObjectWrapper, HasGetValueTypeHash)
	float                                              TimePerFire;                                               // 0x0588(0x0004) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              ProjectileSpeed;                                           // 0x05A4(0x0004) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              ProjectileGravityScale;                                    // 0x05A8(0x0004) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	FFloatRange PitchRange;
	FFloatRange YawRange;
	float                                              PitchSpeed;                                                // 0x05B4(0x0004) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              YawSpeed;                                                  // 0x05B8(0x0004) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	unsigned char                                      UnknownData_5UHE[0x4];                                     // 0x05BC(0x0004) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	class UClass* CameraShake;                                               // 0x05C0(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, UObjectWrapper, HasGetValueTypeHash)
	float                                              ShakeInnerRadius;                                          // 0x05C8(0x0004) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              ShakeOuterRadius;                                          // 0x05CC(0x0004) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              CannonFiredAINoiseRange;                                   // 0x05D0(0x0004) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	struct FName                                       AINoiseTag;                                                // 0x05D4(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	unsigned char                                      UnknownData_WIJI[0x4];                                     // 0x05DC(0x0004) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	char pad_956335424[0x18];
	unsigned char                                      UnknownData_YCZ3[0x20];                                    // 0x05DC(0x0020) FIX WRONG TYPE SIZE OF PREVIOUS PROPERTY
	char pad_9335443224[0x18];
	unsigned char                                      UnknownData_BEDV[0x20];                                    // 0x0618(0x0020) FIX WRONG TYPE SIZE OF PREVIOUS PROPERTY
	float                                              DefaultFOV;                                                // 0x0650(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              AimFOV;                                                    // 0x0654(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              IntoAimBlendSpeed;                                         // 0x0658(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              OutOfAimBlendSpeed;                                        // 0x065C(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWwiseEvent* FireSfx;                                                   // 0x0660(0x0008) (Edit, BlueprintVisible, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWwiseEvent* DryFireSfx;                                                // 0x0668(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWwiseEvent* LoadingSfx_Play;                                           // 0x0670(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWwiseEvent* LoadingSfx_Stop;                                           // 0x0678(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWwiseEvent* UnloadingSfx_Play;                                         // 0x0680(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWwiseEvent* UnloadingSfx_Stop;                                         // 0x0688(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWwiseEvent* LoadedPlayerSfx;                                           // 0x0690(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWwiseEvent* UnloadedPlayerSfx;                                         // 0x0698(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWwiseEvent* FiredPlayerSfx;                                            // 0x06A0(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWwiseObjectPoolWrapper* SfxPool;                                                   // 0x06A8(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWwiseEvent* StartPitchMovement;                                        // 0x06B0(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWwiseEvent* StopPitchMovement;                                         // 0x06B8(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWwiseEvent* StartYawMovement;                                          // 0x06C0(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWwiseEvent* StopYawMovement;                                           // 0x06C8(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWwiseEvent* StopMovementAtEnd;                                         // 0x06D0(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWwiseObjectPoolWrapper* SfxMovementPool;                                           // 0x06D8(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UObject* FuseVfxFirstPerson;                                        // 0x06E0(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UObject* FuseVfxThirdPerson;                                        // 0x06E8(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UObject* MuzzleFlashVfxFirstPerson;                                 // 0x06F0(0x0008) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UObject* MuzzleFlashVfxThirdPerson;                                 // 0x06F8(0x0008) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	struct FName                                       FuseSocketName;                                            // 0x0700(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	struct FName                                       BarrelSocketName;                                          // 0x0708(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UClass* RadialCategoryFilter;                                      // 0x0710(0x0008) (Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, UObjectWrapper, HasGetValueTypeHash)
	class UClass* DefaultLoadedItemDesc;                                     // 0x0718(0x0008) (Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, UObjectWrapper, HasGetValueTypeHash)
	float                                              ClientRotationBlendTime;                                   // 0x0720(0x0004) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	unsigned char                                      UnknownData_5WSD[0x4];                                     // 0x0724(0x0004) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	class AItemInfo* LoadedItemInfo;                                            // 0x0740(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, Protected, HasGetValueTypeHash)
	unsigned char                                      UnknownData_LECI[0xC];                                     // 0x0730(0x000C) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	float                                              ServerPitch;                                               // 0x0754(0x0004) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	float                                              ServerYaw;                                                 // 0x0758(0x0004) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)


	bool IsReadyToFire() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.Cannon.IsReadyToFire");
		bool is_ready = true;
		ProcessEvent(this, fn, &is_ready);
		return is_ready;
	}

	void Fire() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.Cannon.Fire");
		ProcessEvent(this, fn, nullptr);
	}

	void ForceAimCannon(float Pitch, float Yaw)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Athena.Cannon.ForceAimCannon");
		struct {
			float Pitch;
			float Yaw;
		} params;
		params.Pitch = Pitch;
		params.Yaw = Yaw;
		ProcessEvent(this, fn, &params);
	}

	void Server_Fire(float Pitch, float Yaw)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Athena.Cannon.Server_Fire");
		struct {
			float Pitch;
			float Yaw;
		} params;
		params.Pitch = Pitch;
		params.Yaw = Yaw;
		ProcessEvent(this, fn, &params);
	}

};

struct FStorageContainerNode {
	struct UClass* ItemDesc; // 0x00(0x08)
	int32_t NumItems; // 0x08(0x04)
	char UnknownData_C[0x4]; // 0x0c(0x04)
};

struct AHarpoonLauncher {
	char pad[0x0860];
	FRotator AimRelativeAngularLimitsDegrees;	// 0x0860

	void Server_RequestAim(float InPitch, float InYaw)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Athena.HarpoonLauncher.Server_RequestAim");
		struct {
			float InPitch;
			float InYaw;
		} params;
		params.InPitch = InPitch;
		params.InYaw = InYaw;
		ProcessEvent(this, fn, &params);
	}
};


struct UInventoryManipulatorComponent {
	bool ConsumeItem(ACharacter* item) {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.InventoryManipulatorComponent.ConsumeItem");

		struct
		{
			ACharacter* item;
			bool ReturnValue;
		} params;

		params.item = item;
		params.ReturnValue = false;

		ProcessEvent(this, fn, &params);

		return params.ReturnValue;
	}
};


class ACharacter : public UObject {
public:

	char pad1[0x3C8];	//0x28 from inherit
	APlayerState* PlayerState;  // 0x03F0
	char pad2[0x10];
	AController* Controller; // 0x0408
	char pad3[0x38];
	USkeletalMeshComponent* Mesh; // 0x0448
	UCharacterMovementComponent* CharacterMovement; // 0x0708
	char pad4[0x3C8];
	UWieldedItemComponent* WieldedItemComponent; // 0x0820
	char pad43[0x8];
	UInventoryManipulatorComponent* InventoryManipulatorComponent; // 0x0830
	char pad5[0x10];
	UHealthComponent* HealthComponent; // 0x0848
	char pad6[0x04E8];
	UDrowningComponent* DrowningComponent; // 0x0D38

	void ReceiveTick(float DeltaSeconds)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.ActorComponent.ReceiveTick");
		ProcessEvent(this, fn, &DeltaSeconds);
	}

	void GetActorBounds(bool bOnlyCollidingComponents, FVector& Origin, FVector& BoxExtent) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.GetActorBounds");
		struct
		{
			bool bOnlyCollidingComponents = false;
			FVector Origin;
			FVector BoxExtent;
		} params;

		params.bOnlyCollidingComponents = bOnlyCollidingComponents;

		ProcessEvent(this, fn, &params);

		Origin = params.Origin;
		BoxExtent = params.BoxExtent;
	}

	ACharacter* GetCurrentShip() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.AthenaCharacter.GetCurrentShip");
		ACharacter* ReturnValue;
		ProcessEvent(this, fn, &ReturnValue);
		return ReturnValue;
	}

	ACharacter* GetAttachParentActor() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.GetAttachParentActor");
		ACharacter* ReturnValue;
		ProcessEvent(this, fn, &ReturnValue);
		return ReturnValue;
	};

	ACharacter* GetParentActor() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.GetParentActor");
		ACharacter* ReturnValue;
		ProcessEvent(this, fn, &ReturnValue);
		return ReturnValue;
	};

	ACharacter* GetWieldedItem() {
		if (!WieldedItemComponent) return nullptr;
		return WieldedItemComponent->CurrentlyWieldedItem;
	}

	FVector GetVelocity() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.GetVelocity");
		FVector velocity;
		ProcessEvent(this, fn, &velocity);
		return velocity;
	}

	AItemInfo* GetItemInfo() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.ItemProxy.GetItemInfo");
		AItemInfo* info = nullptr;
		ProcessEvent(this, fn, &info);
		return info;
	}

	void CureAllAilings() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.AthenaCharacter.CureAllAilings");
		ProcessEvent(this, fn, nullptr);
	}

	void Kill(uint8_t DeathType) {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.AthenaCharacter.Kill");
		ProcessEvent(this, fn, &DeathType);
	}

	bool IsDead() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.AthenaCharacter.IsDead");
		bool isDead = true;
		ProcessEvent(this, fn, &isDead);
		return isDead;
	}

	bool IsInWater() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.AthenaCharacter.IsInWater");
		bool isInWater = false;
		ProcessEvent(this, fn, &isInWater);
		return isInWater;
	}

	FRotator K2_GetActorRotation() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.K2_GetActorRotation");
		FRotator params;
		ProcessEvent(this, fn, &params);
		return params;
	}

	FVector K2_GetActorLocation() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.K2_GetActorLocation");
		FVector params;
		ProcessEvent(this, fn, &params);
		return params;
	}

	FVector GetActorForwardVector() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.GetActorForwardVector");
		FVector params;
		ProcessEvent(this, fn, &params);
		return params;
	}

	FVector GetActorUpVector() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.GetActorUpVector");
		FVector params;
		ProcessEvent(this, fn, &params);
		return params;
	}

	inline bool isSkeleton() {
		static auto obj = UObject::FindClass("Class Athena.AthenaAICharacter");
		return IsA(obj);
	}

	inline bool isPlayer() {
		static auto obj = UObject::FindClass("Class Athena.AthenaPlayerCharacter");
		return IsA(obj);
	}

	inline bool isPuzzleVault() {
		static auto obj = UObject::FindClass("Class Athena.PuzzleVault");
		return IsA(obj);
	}

	inline bool isShip() {
		static auto obj = UObject::FindClass("Class Athena.Ship");
		return IsA(obj);
	}

	inline bool isWheel() {
		static auto obj = UObject::FindClass("Class Athena.Wheel");
		return IsA(obj);
	}

	inline bool isCannonProjectile() {
		static auto obj = UObject::FindClass("Class Athena.CannonProjectile");
		return IsA(obj);
	}

	inline bool isMapTable() {
		static auto obj = UObject::FindClass("Class Athena.MapTable");
		return IsA(obj);
	}

	inline bool isHarpoon() {
		static auto obj = UObject::FindClass("Class Athena.HarpoonLauncher");
		return IsA(obj);
	}

	inline bool isCannon() {
		static auto obj = UObject::FindClass("Class Athena.Cannon");
		return IsA(obj);
	}

	inline bool isFarShip() {
		static auto obj = UObject::FindClass("Class Athena.ShipNetProxy");
		return IsA(obj);
	}

	inline bool isItem() {
		static auto obj = UObject::FindClass("Class Athena.ItemProxy");
		return IsA(obj);
	}

	inline bool isShipwreck() {
		static auto obj = UObject::FindClass("Class Athena.Shipwreck");
		return IsA(obj);

	}

	inline bool isShark() {
		static auto obj = UObject::FindClass("Class Athena.SharkPawn");
		return IsA(obj);
	}

	inline bool isAnimal() {
		static auto obj = UObject::FindClass("Class AthenaAI.Fauna");
		return IsA(obj);
	}

	bool isWeapon() {
		static auto obj = UObject::FindClass("Class Athena.ProjectileWeapon");
		return IsA(obj);
	}

	bool isSword() {
		static auto obj = UObject::FindClass("Class Athena.MeleeWeapon");
		return IsA(obj);
	}

	bool isBarrel() {
		static auto obj = UObject::FindClass("Class Athena.StorageContainer");
		return IsA(obj);
	}
	bool isWorldSettings() {
		static auto obj = UObject::FindClass("Class Engine.WorldSettings");
		return IsA(obj);
	}
	bool isBuriedTreasure() {
		static auto obj = UObject::FindClass("Class Athena.BuriedTreasureLocation");
		return IsA(obj);
	}

	FAIEncounterSpecification GetAIEncounterSpec() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.AthenaAICharacter.GetAIEncounterSpec");
		FAIEncounterSpecification spec;
		ProcessEvent(this, fn, &spec);
		return spec;
	}
	AHullDamage* GetHullDamage() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.Ship.GetHullDamage");
		AHullDamage* params = nullptr;
		ProcessEvent(this, fn, &params);
		return params;
	}	
	AShipInternalWater* GetInternalWater() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.Ship.GetInternalWater");
		AShipInternalWater* params = nullptr;
		ProcessEvent(this, fn, &params);
		return params;
	}

	float GetMinWheelAngle() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.Wheel.GetMinWheelAngle");
		float angle = 0.f;
		ProcessEvent(this, fn, &angle);
		return angle;
	}
	
	float GetMaxWheelAngle() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.Wheel.GetMaxWheelAngle");
		float angle = 0.f;
		ProcessEvent(this, fn, &angle);
		return angle;
	}

	void ForceSetWheelAngle(float Angle) {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.Wheel.ForceSetWheelAngle");
		ProcessEvent(this, fn, &Angle);
	}

	bool CanJump() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Character.CanJump");
		bool can_jump = false;
		ProcessEvent(this, fn, &can_jump);
		return can_jump;
	}

	void Jump() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Character.Jump");
		ProcessEvent(this, fn, nullptr);
	}

	void LaunchCharacter(FVector& LaunchVelocity, bool bXYOverride, bool bZOverride) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Character.LaunchCharacter");
		
		struct
		{
			FVector LaunchVelocity;
			bool bXYOverride;
			bool bZOverride;
		} params;

		params.LaunchVelocity = LaunchVelocity;
		params.bXYOverride = bXYOverride;
		params.bZOverride = bZOverride;

		ProcessEvent(this, fn, &params);
	}
	

};



class UKismetMathLibrary {
private:
	static inline UClass* defaultObj;
public:
	static bool Init() {
		return defaultObj = UObject::FindObject<UClass>("Class Engine.KismetMathLibrary");
	}
	static FRotator NormalizedDeltaRotator(const struct FRotator& A, const struct FRotator& B) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.KismetMathLibrary.NormalizedDeltaRotator");

		struct
		{
			struct FRotator                A;
			struct FRotator                B;
			struct FRotator                ReturnValue;
		} params;

		params.A = A;
		params.B = B;

		ProcessEvent(defaultObj, fn, &params);

		return params.ReturnValue;

	};
	static FRotator FindLookAtRotation(const FVector& Start, const FVector& Target) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.KismetMathLibrary.FindLookAtRotation");

		struct {
			FVector Start;
			FVector Target;
			FRotator ReturnValue;
		} params;
		params.Start = Start;
		params.Target = Target;

		ProcessEvent(defaultObj, fn, &params);
		return params.ReturnValue;
	}

	static FVector Conv_RotatorToVector(const struct FRotator& InRot) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.KismetMathLibrary.Conv_RotatorToVector");

		struct
		{
			struct FRotator                InRot;
			struct FVector                 ReturnValue;
		} params;
		params.InRot = InRot;

		ProcessEvent(defaultObj, fn, &params);		
		return params.ReturnValue;
	}

	static bool LineTraceSingle_NEW(class UObject* WorldContextObject, const struct FVector& Start, const struct FVector& End, ETraceTypeQuery TraceChannel, bool bTraceComplex, TArray<class AActor*> ActorsToIgnore, EDrawDebugTrace DrawDebugType, bool bIgnoreSelf, struct FHitResult* OutHit)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.KismetSystemLibrary.LineTraceSingle_NEW");

		struct
		{
			class UObject* WorldContextObject;
			struct FVector                 Start;
			struct FVector                 End;
			ETraceTypeQuery				   TraceChannel;
			bool                           bTraceComplex;
			TArray<class AActor*>          ActorsToIgnore;
			EDrawDebugTrace				   DrawDebugType;
			struct FHitResult              OutHit;
			bool                           bIgnoreSelf;
			bool                           ReturnValue;
		} params;

		params.WorldContextObject = WorldContextObject;
		params.Start = Start;
		params.End = End;
		params.TraceChannel = TraceChannel;
		params.bTraceComplex = bTraceComplex;
		params.ActorsToIgnore = ActorsToIgnore;
		params.DrawDebugType = DrawDebugType;
		params.bIgnoreSelf = bIgnoreSelf;

		ProcessEvent(defaultObj, fn, &params);

		if (OutHit != nullptr)
			*OutHit = params.OutHit;

		return params.ReturnValue;
	}

	static void DrawDebugBox(UObject* WorldContextObject, const FVector& Center, const FVector& Extent, const FLinearColor& LineColor, const FRotator& Rotation, float Duration) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.KismetSystemLibrary.DrawDebugBox");
		struct
		{
			UObject* WorldContextObject = nullptr;
			FVector Center;
			FVector Extent;
			FLinearColor LineColor;
			FRotator Rotation;
			float Duration = INFINITY;
		} params;

		params.WorldContextObject = WorldContextObject;
		params.Center = Center;
		params.Extent = Extent;
		params.LineColor = LineColor;
		params.Rotation = Rotation;
		params.Duration = Duration;
		ProcessEvent(defaultObj, fn, &params);
	}
	static void DrawDebugArrow(UObject* WorldContextObject, const FVector& LineStart, const FVector& LineEnd, float ArrowSize, const FLinearColor& LineColor, float Duration) {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.KismetSystemLibrary.DrawDebugBox");
		struct
		{
			class UObject* WorldContextObject = nullptr;
			struct FVector LineStart;
			struct FVector LineEnd;
			float ArrowSize = 1.f;
			struct FLinearColor LineColor;
			float Duration = 1.f;
		} params;

		params.WorldContextObject = WorldContextObject;
		params.LineStart = LineStart;
		params.LineEnd = LineEnd;
		params.ArrowSize = ArrowSize;
		params.LineColor = LineColor;
		params.Duration = Duration;

		ProcessEvent(defaultObj, fn, &params);
	}
};

struct UCrewFunctions {
private:
	static inline UClass* defaultObj;
public:
	static bool Init() {
		return defaultObj = UObject::FindObject<UClass>("Class Athena.CrewFunctions");
	}
	static bool AreCharactersInSameCrew(ACharacter* Player1, ACharacter* Player2) {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.CrewFunctions.AreCharactersInSameCrew");
		struct
		{
			ACharacter* Player1;
			ACharacter* Player2;
			bool ReturnValue;
		} params;
		params.Player1 = Player1;
		params.Player2 = Player2;
		ProcessEvent(defaultObj, fn, &params);
		return params.ReturnValue;
	}
};

struct UPlayer {
	char UnknownData00[0x30];
	AController* PlayerController;
};

struct UGameInstance {
	char UnknownData00[0x38];
	TArray<UPlayer*> LocalPlayers; // 0x38
};

struct ULevel {
	char UnknownData00[0xA0];
	TArray<ACharacter*> AActors;
};

struct UWorld {
	static inline UWorld** GWorld = nullptr;
	char pad[0x30];
	ULevel* PersistentLevel; // 0x0030
	char pad_0028[0x20];
	AAthenaGameState* GameState; //0x0058
	char pad_0060[0xF0];
	TArray<ULevel*> Levels; //0x0150
	char pad_0160[0x50];
	ULevel* CurrentLevel; //0x01B0
	char pad_01B8[0x8];
	UGameInstance* GameInstance; //0x01C0
};

// Class Engine.PrimitiveComponent
// Size: 0x590 (Inherited: 0x2b0)
struct UPrimitiveComponent : USceneComponent {
	char UnknownData_2B0[0x8]; // 0x2b0(0x08)
	float MinDrawDistance; // 0x2b8(0x04)
	char UnknownData_2BC[0x4]; // 0x2bc(0x04)
	float LDMaxDrawDistance; // 0x2c0(0x04)
	float CachedMaxDrawDistance; // 0x2c4(0x04)
	char DepthPriorityGroup; // 0x2c8(0x01)
	char ViewOwnerDepthPriorityGroup; // 0x2c9(0x01)
	char UnknownData_2CA[0x2]; // 0x2ca(0x02)
	char bAlwaysCreatePhysicsState : 1; // 0x2cc(0x01)
	char bGenerateOverlapEvents : 1; // 0x2cc(0x01)
	char bMultiBodyOverlap : 1; // 0x2cc(0x01)
	char bCheckAsyncSceneOnMove : 1; // 0x2cc(0x01)
	char bTraceComplexOnMove : 1; // 0x2cc(0x01)
	char bReturnMaterialOnMove : 1; // 0x2cc(0x01)
	char bUseViewOwnerDepthPriorityGroup : 1; // 0x2cc(0x01)
	char bAllowCullDistanceVolume : 1; // 0x2cc(0x01)
	char bHasMotionBlurVelocityMeshes : 1; // 0x2cd(0x01)
	char bRenderInMainPass : 1; // 0x2cd(0x01)
	char bRenderInCustomPrePass : 1; // 0x2cd(0x01)
	char bReflected : 1; // 0x2cd(0x01)
	char UnknownData_2CD_4 : 1; // 0x2cd(0x01)
	char bReflectedOnLowQuality : 1; // 0x2cd(0x01)
	char bFFTWaterMask : 1; // 0x2cd(0x01)
	char bVolumeFogMask : 1; // 0x2cd(0x01)
	char UnknownData_2CE_0 : 1; // 0x2ce(0x01)
	char bAffectsFlatWater : 1; // 0x2ce(0x01)
	char bGPUParticlesKillPlane : 1; // 0x2ce(0x01)
	char bDontCull : 1; // 0x2ce(0x01)
	char bDontSizeOnScreenCull : 1; // 0x2ce(0x01)
	char UnknownData_2CE_5 : 3; // 0x2ce(0x01)
	char UnknownData_2CF[0x1]; // 0x2cf(0x01)
	float OverriddenShadowMinTexelSize; // 0x2d0(0x04)
	bool bOverrideShadowMinSizeCulling; // 0x2d4(0x01)
	bool bOverrideShadowCascadesExclusion; // 0x2d5(0x01)
	char ExcludedShadowCascades; // 0x2d6(0x01)
	char UnknownData_2D7[0x1]; // 0x2d7(0x01)
	char bReceivesDecals : 1; // 0x2d8(0x01)
	char bOwnerNoSee : 1; // 0x2d8(0x01)
	char bOnlyOwnerSee : 1; // 0x2d8(0x01)
	char bTreatAsBackgroundForOcclusion : 1; // 0x2d8(0x01)
	char bIsACloud : 1; // 0x2d8(0x01)
	char bUseAsOccluder : 1; // 0x2d8(0x01)
	char bSelectable : 1; // 0x2d8(0x01)
	char bForceMipStreaming : 1; // 0x2d8(0x01)
	char bHasPerInstanceHitProxies : 1; // 0x2d9(0x01)
	char CastShadow : 1; // 0x2d9(0x01)
	char bAffectDynamicIndirectLighting : 1; // 0x2d9(0x01)
	char bUseFarCascadeLPVBiasMultiplier : 1; // 0x2d9(0x01)
	char bAffectDistanceFieldLighting : 1; // 0x2d9(0x01)
	char bCastDynamicShadow : 1; // 0x2d9(0x01)
	char bCastStaticShadow : 1; // 0x2d9(0x01)
	char bCastVolumetricTranslucentShadow : 1; // 0x2d9(0x01)
	char bSelfShadowOnly : 1; // 0x2da(0x01)
	char bCastFarShadow : 1; // 0x2da(0x01)
	char bCastInsetShadow : 1; // 0x2da(0x01)
	char bCastCinematicShadow : 1; // 0x2da(0x01)
	char bCastHiddenShadow : 1; // 0x2da(0x01)
	char bCastShadowAsTwoSided : 1; // 0x2da(0x01)
	char bCastShadowOnLowQuality : 1; // 0x2da(0x01)
	char bLightAsIfStatic : 1; // 0x2da(0x01)
	char bLightAttachmentsAsGroup : 1; // 0x2db(0x01)
	char UnknownData_2DB_1 : 7; // 0x2db(0x01)
	char IndirectLightingCacheQuality; // 0x2dc(0x01)
	bool bHasCachedStaticLighting; // 0x2dd(0x01)
	bool bStaticLightingBuildEnqueued; // 0x2de(0x01)
	char UnknownData_2DF[0x1]; // 0x2df(0x01)
	char bIgnoreRadialImpulse : 1; // 0x2e0(0x01)
	char bIgnoreRadialForce : 1; // 0x2e0(0x01)
	char AlwaysLoadOnClient : 1; // 0x2e0(0x01)
	char AlwaysLoadOnServer : 1; // 0x2e0(0x01)
	char bUseEditorCompositing : 1; // 0x2e0(0x01)
	char bRenderCustomDepth : 1; // 0x2e0(0x01)
	char bAllowVelocityInMaterial : 1; // 0x2e0(0x01)
	char UnknownData_2E1[0x3]; // 0x2e1(0x03)
	int32_t CustomDepthStencilValue; // 0x2e4(0x04)
	int32_t TranslucencySortPriority; // 0x2e8(0x04)
	int32_t VisibilityId; // 0x2ec(0x04)
	char UnknownData_2F0[0x4]; // 0x2f0(0x04)
	float LpvBiasMultiplier; // 0x2f4(0x04)
	float FarCascadeLPVBiasMultiplier; // 0x2f8(0x04)
	float LpvIntensityMultiplier; // 0x2fc(0x04)
	char bAffectRain : 1; // 0x490(0x01)
	char bCanEverAffectNavigation : 1; // 0x490(0x01)
	char UnknownData_490_2 : 1; // 0x490(0x01)
	char bSkipRenderingInOuterLPVCascades : 1; // 0x490(0x01)
	char bEnableMergeCollisionComponents : 1; // 0x490(0x01)
	char bVisibleWhenAboveWaterAndPlayerUnderwater : 1; // 0x490(0x01)
	char bVisibleWhenAboveWaterAndPlayerAbove : 1; // 0x490(0x01)
	char bVisibleWhenUnderwaterAndPlayerAbove : 1; // 0x490(0x01)
	char bVisibleWhenUnderwaterAndPlayerUnderwater : 1; // 0x491(0x01)
	char bCanRenderAboveAndBelowWaterAtSameTime : 1; // 0x491(0x01)
	char UnknownData_491_2 : 6; // 0x491(0x01)
	char UnknownData_492[0x6]; // 0x492(0x06)
	float BoundsScale; // 0x498(0x04)
	float OcclusionBoundsScale; // 0x49c(0x04)
	float LastRenderTime; // 0x4a0(0x04)
	bool bGPUVisibility; // 0x4a4(0x01)
	char bHasCustomNavigableGeometry; // 0x4a5(0x01)
	char CanCharacterStepUpOn; // 0x4a6(0x01)
	char UnknownData_4A7[0x49]; // 0x4a7(0x49)
	char UnknownData_4F5[0x33]; // 0x4f5(0x33)
	struct UPrimitiveComponent* LODParentPrimitive; // 0x528(0x08)
	struct UPrimitiveComponent* MergedCollisionComponentParent; // 0x580(0x08)
	char UnknownData_588[0x8]; // 0x588(0x08)

	void WakeRigidBody(struct FName BoneName); // Function Engine.PrimitiveComponent.WakeRigidBody // Native|Public|BlueprintCallable // @ game+0x2bce8c0
	void WakeAllRigidBodies(); // Function Engine.PrimitiveComponent.WakeAllRigidBodies // Native|Public|BlueprintCallable // @ game+0x2bce8a0
	void SetWalkableSlopeOverride(struct FWalkableSlopeOverride NewOverride); // Function Engine.PrimitiveComponent.SetWalkableSlopeOverride // Final|Native|Public|HasOutParms|BlueprintCallable // @ game+0x2bcd770
	void SetTranslucentSortPriority(int32_t NewTranslucentSortPriority); // Function Engine.PrimitiveComponent.SetTranslucentSortPriority // Final|Native|Public|BlueprintCallable // @ game+0x2bcd430
	void SetSimulatePhysics(bool bSimulate); // Function Engine.PrimitiveComponent.SetSimulatePhysics // Native|Public|BlueprintCallable // @ game+0x2bccb50
	void SetRenderInMainPass(bool bValue); // Function Engine.PrimitiveComponent.SetRenderInMainPass // Final|Native|Public|BlueprintCallable // @ game+0x2bcc9a0
	void SetRenderCustomDepth(bool bValue); // Function Engine.PrimitiveComponent.SetRenderCustomDepth // Final|Native|Public|BlueprintCallable // @ game+0x2bcc910
	void SetPhysMaterialOverride(struct UPhysicalMaterial* NewPhysMaterial); // Function Engine.PrimitiveComponent.SetPhysMaterialOverride // Native|Public|BlueprintCallable // @ game+0x2bcbe50
	void SetPhysicsMaxAngularVelocity(float NewMaxAngVel, bool bAddToCurrent, struct FName BoneName); // Function Engine.PrimitiveComponent.SetPhysicsMaxAngularVelocity // Final|Native|Public|BlueprintCallable // @ game+0x2bcc250
	void SetPhysicsLinearVelocity(struct FVector NewVel, bool bAddToCurrent, struct FName BoneName); // Function Engine.PrimitiveComponent.SetPhysicsLinearVelocity // Final|Native|Public|HasDefaults|BlueprintCallable // @ game+0x2bcc140
	void SetPhysicsAngularVelocity(struct FVector NewAngVel, bool bAddToCurrent, struct FName BoneName); // Function Engine.PrimitiveComponent.SetPhysicsAngularVelocity // Final|Native|Public|HasDefaults|BlueprintCallable // @ game+0x2bcbee0
	void SetOwnerNoSee(bool bNewOwnerNoSee); // Function Engine.PrimitiveComponent.SetOwnerNoSee // Final|Native|Public|BlueprintCallable // @ game+0x2bcbdc0
	void SetOnlyOwnerSee(bool bNewOnlyOwnerSee); // Function Engine.PrimitiveComponent.SetOnlyOwnerSee // Final|Native|Public|BlueprintCallable // @ game+0x2bcbcb0
	void SetNotifyRigidBodyCollision(bool bNewNotifyRigidBodyCollision); // Function Engine.PrimitiveComponent.SetNotifyRigidBodyCollision // Native|Public|BlueprintCallable // @ game+0x2bcbc20
	void SetMaterial(int32_t ElementIndex, struct UMaterialInterface* Material); // Function Engine.PrimitiveComponent.SetMaterial // Native|Public|BlueprintCallable // @ game+0x2bcb840
	void SetMassScale(struct FName BoneName, float InMassScale); // Function Engine.PrimitiveComponent.SetMassScale // Native|Public|BlueprintCallable // @ game+0x2bcb6f0
	void SetMassOverrideInKg(struct FName BoneName, float MassInKg, bool bOverrideMass); // Function Engine.PrimitiveComponent.SetMassOverrideInKg // Native|Public|BlueprintCallable // @ game+0x2bcb5f0
	void SetLockedAxis(char LockedAxis); // Function Engine.PrimitiveComponent.SetLockedAxis // Native|Public|BlueprintCallable // @ game+0x2bcb4e0
	void SetLinearDamping(float InDamping); // Function Engine.PrimitiveComponent.SetLinearDamping // Native|Public|BlueprintCallable // @ game+0x2bcaca0
	void SetGenerateOverlapEvents(bool bEnable); // Function Engine.PrimitiveComponent.SetGenerateOverlapEvents // Final|Native|Public|BlueprintCallable // @ game+0x2bca420
	void SetEnableGravity(bool bGravityEnabled); // Function Engine.PrimitiveComponent.SetEnableGravity // Native|Public|BlueprintCallable // @ game+0x2bca040
	void SetCustomPrimitiveVector(int32_t Index, struct FVector Value); // Function Engine.PrimitiveComponent.SetCustomPrimitiveVector // Final|Native|Public|HasDefaults|BlueprintCallable // @ game+0x2bc9c20
	void SetCustomPrimitiveScalar(int32_t Index, float Value); // Function Engine.PrimitiveComponent.SetCustomPrimitiveScalar // Final|Native|Public|BlueprintCallable // @ game+0x2bc9b50
	void SetCustomDepthStencilValue(int32_t Value); // Function Engine.PrimitiveComponent.SetCustomDepthStencilValue // Final|Native|Public|BlueprintCallable // @ game+0x2bc9970
	void SetCullDistance(float NewCullDistance); // Function Engine.PrimitiveComponent.SetCullDistance // Final|Native|Public|BlueprintCallable // @ game+0x2bc9870
	void SetConstraintMode(char ConstraintMode); // Function Engine.PrimitiveComponent.SetConstraintMode // Native|Public|BlueprintCallable // @ game+0x2bc94e0
	void SetCollisionResponseToChannel(char Channel, char NewResponse); // Function Engine.PrimitiveComponent.SetCollisionResponseToChannel // Final|Native|Public|BlueprintCallable // @ game+0x2bc9130
	void SetCollisionResponseToAllChannels(char NewResponse); // Function Engine.PrimitiveComponent.SetCollisionResponseToAllChannels // Final|Native|Public|BlueprintCallable // @ game+0x2bc90b0
	void SetCollisionProfileName(struct FName InCollisionProfileName); // Function Engine.PrimitiveComponent.SetCollisionProfileName // Native|Public|BlueprintCallable // @ game+0x2bc9020
	void SetCollisionObjectType(char Channel); // Function Engine.PrimitiveComponent.SetCollisionObjectType // Final|Native|Public|BlueprintCallable // @ game+0x2bc8fa0
	void SetCollisionEnabled(char NewType); // Function Engine.PrimitiveComponent.SetCollisionEnabled // Native|Public|BlueprintCallable // @ game+0x2bc8f20
	void SetCenterOfMass(struct FVector CenterOfMassOffset, struct FName BoneName); // Function Engine.PrimitiveComponent.SetCenterOfMass // Final|Native|Public|HasDefaults|BlueprintCallable // @ game+0x2bc8b80
	void SetCastShadow(bool NewCastShadow); // Function Engine.PrimitiveComponent.SetCastShadow // Final|Native|Public|BlueprintCallable // @ game+0x2bc89d0
	void SetAngularDamping(float InDamping); // Function Engine.PrimitiveComponent.SetAngularDamping // Native|Public|BlueprintCallable // @ game+0x2bc77b0
	void SetAllPhysicsLinearVelocity(struct FVector NewVel, bool bAddToCurrent); // Function Engine.PrimitiveComponent.SetAllPhysicsLinearVelocity // Native|Public|HasDefaults|BlueprintCallable // @ game+0x2bc76d0
	void SetAllMassScale(float InMassScale); // Function Engine.PrimitiveComponent.SetAllMassScale // Native|Public|BlueprintCallable // @ game+0x2bc72b0
	struct FVector ScaleByMomentOfInertia(struct FVector InputVector, struct FName BoneName); // Function Engine.PrimitiveComponent.ScaleByMomentOfInertia // Native|Public|HasDefaults|BlueprintCallable|BlueprintPure|Const // @ game+0x2bc6b30
	void PutRigidBodyToSleep(struct FName BoneName); // Function Engine.PrimitiveComponent.PutRigidBodyToSleep // Final|Native|Public|BlueprintCallable // @ game+0x2bc6500
	bool K2_LineTraceComponent(struct FVector TraceStart, struct FVector TraceEnd, bool bTraceComplex, bool bShowTrace, struct FVector HitLocation, struct FVector HitNormal, struct FName BoneName); // Function Engine.PrimitiveComponent.K2_LineTraceComponent // Final|Native|Public|HasOutParms|HasDefaults|BlueprintCallable // @ game+0x2bc4990
	bool IsOverlappingComponent(struct UPrimitiveComponent* OtherComp); // Function Engine.PrimitiveComponent.IsOverlappingComponent // Final|Native|Public|BlueprintCallable|BlueprintPure|Const // @ game+0x2bc45a0
	bool IsOverlappingActor(struct AActor* Other); // Function Engine.PrimitiveComponent.IsOverlappingActor // Final|Native|Public|BlueprintCallable|BlueprintPure|Const // @ game+0x2bc4510
	bool IsGravityEnabled(); // Function Engine.PrimitiveComponent.IsGravityEnabled // Native|Public|BlueprintCallable|BlueprintPure|Const // @ game+0x2bc4340
	void IgnoreActorWhenMoving(struct AActor* Actor, bool bShouldIgnore); // Function Engine.PrimitiveComponent.IgnoreActorWhenMoving // Final|Native|Public|BlueprintCallable // @ game+0x2bc3ef0
	struct FWalkableSlopeOverride GetWalkableSlopeOverride(); // Function Engine.PrimitiveComponent.GetWalkableSlopeOverride // Final|Native|Public|BlueprintCallable|BlueprintPure|Const // @ game+0x2bc3720
	struct FVector GetPhysicsLinearVelocityAtPoint(struct FVector Point, struct FName BoneName); // Function Engine.PrimitiveComponent.GetPhysicsLinearVelocityAtPoint // Final|Native|Public|HasDefaults|BlueprintCallable // @ game+0x2bc1410
	struct FVector GetPhysicsLinearVelocity(struct FName BoneName); // Function Engine.PrimitiveComponent.GetPhysicsLinearVelocity // Final|Native|Public|HasDefaults|BlueprintCallable // @ game+0x2bc1370
	struct FVector GetPhysicsAngularVelocity(struct FName BoneName); // Function Engine.PrimitiveComponent.GetPhysicsAngularVelocity // Final|Native|Public|HasDefaults|BlueprintCallable // @ game+0x2bc12d0
	void GetOverlappingComponents(struct TArray<struct UPrimitiveComponent*> InOverlappingComponents); // Function Engine.PrimitiveComponent.GetOverlappingComponents // Final|Native|Public|HasOutParms|BlueprintCallable|BlueprintPure|Const // @ game+0x2bc0c10
	void GetOverlappingActors(struct TArray<struct AActor*> OverlappingActors, struct UClass* ClassFilter); // Function Engine.PrimitiveComponent.GetOverlappingActors // Final|Native|Public|HasOutParms|BlueprintCallable|BlueprintPure|Const // @ game+0x2bc0b10
	struct TArray<struct FOverlapInfo> GetOverlapInfos(); // Function Engine.PrimitiveComponent.GetOverlapInfos // Final|Native|Public|BlueprintCallable|BlueprintPure|Const // @ game+0x2bc08f0
	int32_t GetNumMaterials(); // Function Engine.PrimitiveComponent.GetNumMaterials // Native|Public|BlueprintCallable|BlueprintPure|Const // @ game+0x2bc0830
	struct UMaterialInterface* GetMaterial(int32_t ElementIndex); // Function Engine.PrimitiveComponent.GetMaterial // Native|Public|BlueprintCallable|BlueprintPure|Const // @ game+0x2bc0360
	float GetMassScale(struct FName BoneName); // Function Engine.PrimitiveComponent.GetMassScale // Native|Public|BlueprintCallable|BlueprintPure|Const // @ game+0x2bc02a0
	float GetMass(); // Function Engine.PrimitiveComponent.GetMass // Native|Public|BlueprintCallable|BlueprintPure|Const // @ game+0x2bc0260
	float GetLinearDamping(); // Function Engine.PrimitiveComponent.GetLinearDamping // Native|Public|BlueprintCallable|BlueprintPure|Const // @ game+0x2bbfc90
	struct FVector GetInertiaTensor(struct FName BoneName); // Function Engine.PrimitiveComponent.GetInertiaTensor // Native|Public|HasDefaults|BlueprintCallable|BlueprintPure|Const // @ game+0x2bbfa50
	char GetCollisionResponseToChannel(char Channel); // Function Engine.PrimitiveComponent.GetCollisionResponseToChannel // Native|Public|BlueprintCallable|BlueprintPure|Const // @ game+0x2bbf200
	struct FName GetCollisionProfileName(); // Function Engine.PrimitiveComponent.GetCollisionProfileName // Final|Native|Public|BlueprintCallable // @ game+0x2bbf1c0
	char GetCollisionObjectType(); // Function Engine.PrimitiveComponent.GetCollisionObjectType // Native|Public|BlueprintCallable|BlueprintPure|Const // @ game+0x2bbf190
	float GetClosestPointOnCollision(struct FVector Point, struct FVector OutPointOnBody, struct FName BoneName); // Function Engine.PrimitiveComponent.GetClosestPointOnCollision // Final|Native|Public|HasOutParms|HasDefaults|BlueprintCallable|BlueprintPure|Const // @ game+0x2bbf020
	struct FVector GetCenterOfMass(struct FName BoneName); // Function Engine.PrimitiveComponent.GetCenterOfMass // Final|Native|Public|HasDefaults|BlueprintCallable // @ game+0x2bbef80
	float GetAngularDamping(); // Function Engine.PrimitiveComponent.GetAngularDamping // Native|Public|BlueprintCallable|BlueprintPure|Const // @ game+0x2bbece0
	struct UMaterialInstanceDynamic* CreateDynamicMaterialInstance(int32_t ElementIndex, struct UMaterialInterface* SourceMaterial); // Function Engine.PrimitiveComponent.CreateDynamicMaterialInstance // Native|Public|BlueprintCallable // @ game+0x2bbca40
	struct UMaterialInstanceDynamic* CreateAndSetMaterialInstanceDynamicFromMaterial(int32_t ElementIndex, struct UMaterialInterface* Parent); // Function Engine.PrimitiveComponent.CreateAndSetMaterialInstanceDynamicFromMaterial // Native|Public|BlueprintCallable // @ game+0x2bbc940
	struct UMaterialInstanceDynamic* CreateAndSetMaterialInstanceDynamic(int32_t ElementIndex); // Function Engine.PrimitiveComponent.CreateAndSetMaterialInstanceDynamic // Native|Public|BlueprintCallable // @ game+0x2bbc8a0
	struct TArray<struct AActor*> CopyArrayOfMoveIgnoreActors(); // Function Engine.PrimitiveComponent.CopyArrayOfMoveIgnoreActors // Final|Native|Public|BlueprintCallable // @ game+0x2bbc800
	void ClearMoveIgnoreActors(int32_t InSlack); // Function Engine.PrimitiveComponent.ClearMoveIgnoreActors // Final|Native|Public|BlueprintCallable // @ game+0x2bbc3e0
	bool CanCharacterStepUp(struct APawn* Pawn); // Function Engine.PrimitiveComponent.CanCharacterStepUp // Native|Public|BlueprintCallable|BlueprintPure|Const // @ game+0x2bbc000
	void AddTorque(struct FVector Torque, struct FName BoneName, bool bAccelChange); // Function Engine.PrimitiveComponent.AddTorque // Final|Native|Public|HasDefaults|BlueprintCallable // @ game+0x2bbbc90
	void AddRadialImpulse(struct FVector Origin, float Radius, float Strength, char Falloff, bool bVelChange); // Function Engine.PrimitiveComponent.AddRadialImpulse // Native|Public|HasDefaults|BlueprintCallable // @ game+0x2bbb8d0
	void AddRadialForce(struct FVector Origin, float Radius, float Strength, char Falloff, bool bAccelChange); // Function Engine.PrimitiveComponent.AddRadialForce // Native|Public|HasDefaults|BlueprintCallable // @ game+0x2bbb710
	void AddImpulseAtLocation(struct FVector Impulse, struct FVector Location, struct FName BoneName); // Function Engine.PrimitiveComponent.AddImpulseAtLocation // Native|Public|HasDefaults|BlueprintCallable // @ game+0x2bbb340
	void AddImpulse(struct FVector Impulse, struct FName BoneName, bool bVelChange); // Function Engine.PrimitiveComponent.AddImpulse // Native|Public|HasDefaults|BlueprintCallable // @ game+0x2bbb220
	void AddForceAtLocation(struct FVector Force, struct FVector Location, struct FName BoneName); // Function Engine.PrimitiveComponent.AddForceAtLocation // Native|Public|HasDefaults|BlueprintCallable // @ game+0x2bbaf60
	void AddForce(struct FVector Force, struct FName BoneName, bool bAccelChange); // Function Engine.PrimitiveComponent.AddForce // Native|Public|HasDefaults|BlueprintCallable // @ game+0x2bbae40
	void AddAngularImpulse(struct FVector Impulse, struct FName BoneName, bool bVelChange); // Function Engine.PrimitiveComponent.AddAngularImpulse // Native|Public|HasDefaults|BlueprintCallable // @ game+0x2bba640
};

struct ASkellyFort
{
	char pad[0x0544];
	FVector SkullCloudLoc;  // 0x0544(0x000C) (Edit, ZeroConstructor, IsPlainOldData, NoDestructor, Protected)

};


// Enum Athena.EMeleeWeaponMovementSpeed
enum class EMeleeWeaponMovementSpeed : uint8_t
{
	EMeleeWeaponMovementSpeed__Default = 0,
	EMeleeWeaponMovementSpeed__SlightlySlowed = 1,
	EMeleeWeaponMovementSpeed__Slowed = 2,
	EMeleeWeaponMovementSpeed__EMeleeWeaponMovementSpeed_MAX = 3
};

struct UMeleeAttackDataAsset
{
	char pad[0x0238];
	float ClampYawRange; //0x0238
	float ClampYawRate; //0x023C
};



struct UMeleeWeaponDataAsset
{
	char pad[0x0048];
	class UMeleeAttackDataAsset* HeavyAttack; //0x0048
	char pad2[0x0028];
	EMeleeWeaponMovementSpeed BlockingMovementSpeed; //0x0078
};

struct AMeleeWeapon
{
	char pad[0x07B0];
	struct UMeleeWeaponDataAsset* DataAsset; //0x07B0
};

struct AMapTable
{
	char pad[0x04E0];
	TArray<struct FVector2D> MapPins; // 0x04E0
};


#ifdef _MSC_VER
#pragma pack(pop)
#endif
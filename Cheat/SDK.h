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
	UndoingRepair = 19,
	Wheel = 20,
	Wheel_END = 21,
};

struct FPirateDescription
{

};

struct AAthenaPlayerCharacter {
};

struct APlayerState 
{
	char pad[0x03D0];
	float                                              Score;                                                     // 0x03D0(0x0004) (BlueprintVisible, BlueprintReadOnly, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float											   Ping;                                                      // 0x03D4(0x0001) (BlueprintVisible, BlueprintReadOnly, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	FString							                   PlayerName;                                                // 0x03D8 (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, RepNotify, HasGetValueTypeHash)


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
	char UnknownData_18[0x10];
	//float FOV;
};

struct UFOVHandlerFunctions_GetTargetFOV_Params
{
	class AAthenaPlayerCharacter* Character;
	float ReturnValue;
};

struct AGameCustomizationService_SetTime_Params
{
	int Hours;

	void SetTime(int Hours)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Athena.GameCustomizationService.SetTime");
		AGameCustomizationService_SetTime_Params params;
		params.Hours = Hours;
		ProcessEvent(this, fn, &params);
	}
};

struct UFOVHandlerFunctions_SetTargetFOV_Params
{
	class AAthenaPlayerCharacter* Character;
	float TargetFOV;

	void SetTargetFOV(class AAthenaPlayerCharacter* Character, float TargetFOV)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Athena.FOVHandlerFunctions.SetTargetFOV");
		UFOVHandlerFunctions_SetTargetFOV_Params params;
		params.Character = Character;
		params.TargetFOV = TargetFOV;
		ProcessEvent(this, fn, &params);
	}
};

struct FCameraCacheEntry {
	float TimeStamp;
	char pad[0xc];
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

struct AController_K2_GetPawn_Params
{
	class APawn* ReturnValue;
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
	bool IdleDisconnectEnabled; // 0x14D1

	void SetTime(int Hours)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Athena.GameCustomizationService.SetTime");
		AGameCustomizationService_SetTime_Params params;
		params.Hours = Hours;
		ProcessEvent(this, fn, &params);
	}

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

	APawn* K2_GetPawn() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Controller.K2_GetPawn");
		AController_K2_GetPawn_Params params;
		class APawn* ReturnValue;
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
	int AmmoClipSize; // 0x00(0x04)
	int AmmoCostPerShot; // 0x04(0x04)
	float EquipDuration; // 0x08(0x04)
	float IntoAimingDuration; // 0x0c(0x04)
	float RecoilDuration; // 0x10(0x04)
	float ReloadDuration; // 0x14(0x04)
	struct FProjectileShotParams HipFireProjectileShotParams; // 0x18(0x18)
	struct FProjectileShotParams AimDownSightsProjectileShotParams; // 0x30(0x18)
	int Seed; // 0x48(0x04)
	float ProjectileDistributionMaxAngle; // 0x4c(0x04)
	int NumberOfProjectiles; // 0x50(0x04)
	float ProjectileMaximumRange; // 0x54(0x04)
	float ProjectileDamage; // 0x58(0x04)
	float ProjectileDamageMultiplierAtMaximumRange; // 0x5c(0x04)
	struct UClass* DamagerType; // 0x60(0x08)
	struct UClass* ProjectileId; // 0x68(0x08)
	struct FWeaponProjectileParams AmmoParams; // 0x70(0xa8)
	bool UsesScope; // 0x118(0x01)
	char UnknownData_119[0x3]; // 0x119(0x03)
	float ZoomedRecoilDurationIncrease; // 0x11c(0x04)
	float SecondsUntilZoomStarts; // 0x120(0x04)
	float SecondsUntilPostStarts; // 0x124(0x04)
	float WeaponFiredAINoiseRange; // 0x128(0x04)
	float MaximumRequestPositionDelta; // 0x12c(0x04)
	float MaximumRequestAngleDelta; // 0x130(0x04)
	float TimeoutTolerance; // 0x134(0x04)
	float AimingMoveSpeedScalar; // 0x138(0x04)
	char AimSensitivitySettingCategory; // 0x13c(0x01)
	char UnknownData_13D[0x3]; // 0x13d(0x03)
	float InAimFOV; // 0x140(0x04)
	float BlendSpeed; // 0x144(0x04)
	struct UWwiseEvent* DryFireSfx; // 0x148(0x08)
	struct FName RumbleTag; // 0x160(0x08)
	bool KnockbackEnabled; // 0x168(0x01)
	char UnknownData_169[0x3]; // 0x169(0x03)
	bool StunEnabled; // 0x1bc(0x01)
	char UnknownData_1BD[0x3]; // 0x1bd(0x03)
	float StunDuration; // 0x1c0(0x04)
	struct FVector TargetingOffset; // 0x1c4(0x0c)
};

struct URepairableComponent {
	unsigned char                                      UnknownData_WIVW[0x18];                                    // 0x0118(0x0018) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	float                                              InteractionPointDepthOffset;                               // 0x0160(0x0004) (Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              MaximumRepairAngleToRepairer;                              // 0x0164(0x0004) (Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              MaximumRepairDistance;                                     // 0x0168(0x0004) (Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              RepairTime;                                                // 0x016C(0x0004) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UClass* RepairType;                                                // 0x0170(0x0008) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, UObjectWrapper, HasGetValueTypeHash)
	class USceneComponent* RepairMountParent;                                         // 0x0178(0x0008) (Edit, BlueprintVisible, ExportObject, ZeroConstructor, InstancedReference, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	struct FTransform                                  RepairMountOffset;                                         // 0x0180(0x0030) (Edit, BlueprintVisible, IsPlainOldData, NoDestructor)
	struct FName                                       RepairMountSocket;                                         // 0x01B0(0x0008) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	int                                                MaxDamageLevel;                                            // 0x01B8(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	unsigned char                                      UnknownData_CZX3[0x4];                                     // 0x01BC(0x0004) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	class UClass* AIInteractionType;                                         // 0x01C0(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, UObjectWrapper, HasGetValueTypeHash)
	struct FVector                                     AIInteractionOffset;                                       // 0x01C8(0x000C) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor)
	unsigned char                                      UnknownData_SYYG[0x4];                                     // 0x01D4(0x0004) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	unsigned char                                      UnknownData_EU9A[0x3];                                     // 0x01F1(0x0003) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	int                                                DamageLevel;                                               // 0x01F4(0x0004) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	unsigned char                                      UnknownData_Z10X[0x87];                                    // 0x01F9(0x0087) MISSED OFFSET (PADDING)
};

struct ABucket {
	char pad1[0x0798];
	class UInventoryItemComponent* InventoryItem;                                             // 0x0798(0x0008) (Edit, BlueprintVisible, ExportObject, ZeroConstructor, InstancedReference, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	TArray<struct FBucketContentsInfo>                 BucketContentsInfos;                                       // 0x07A0(0x0010) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance)
	class UWwiseEvent* ScoopSfx;                                                  // 0x07B0(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	struct FName                                       ThrowSocketName;                                           // 0x07B8(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	struct FName                                       DrenchWielderSocketName;                                   // 0x07C0(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              ScoopActionTime;                                           // 0x07C8(0x0004) (Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              ScoopCompleteTime;                                         // 0x07CC(0x0004) (Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              ThrowActionTime;                                           // 0x07D0(0x0004) (Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              ThrowCompleteTime;                                         // 0x07D4(0x0004) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              DrenchWielderActionTime;                                   // 0x07D8(0x0004) (Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              DrenchWielderCompleteTime;                                 // 0x07DC(0x0004) (Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              RequestToleranceTimeOnServer;                              // 0x07E0(0x0004) (Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              ProjectileSpeed;                                           // 0x07E4(0x0004) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              ProjectileAdditionalLiftAngle;                             // 0x07E8(0x0004) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              WaterFillFromShip;                                         // 0x07EC(0x0004) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              WaterFillFromSea;                                          // 0x07F0(0x0004) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              WaterTransferFillAmountModifier;                           // 0x07F4(0x0004) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              ScoopBufferDistance;                                       // 0x07F8(0x0004) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	unsigned char                                      UnknownData_Z3K3[0x3];                                     // 0x07FD(0x0003) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	class UClass* BucketScoopCooldownType;                                   // 0x0800(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, UObjectWrapper, HasGetValueTypeHash)
	class UClass* BucketThrowCooldownType;                                   // 0x0808(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, UObjectWrapper, HasGetValueTypeHash)
	class UClass* BucketDouseCooldownType;                                   // 0x0810(0x0008) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, UObjectWrapper, HasGetValueTypeHash)
	float                                              ThrowLiquidAINoiseRange;                                   // 0x0818(0x0004) (Edit, BlueprintVisible, BlueprintReadOnly, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	unsigned char                                      UnknownData_1WAE[0x4];                                     // 0x081C(0x0004) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	class UNamedNotificationInputComponent* NamedNotificationInputComponent;                           // 0x0820(0x0008) (Edit, ExportObject, ZeroConstructor, DisableEditOnInstance, InstancedReference, IsPlainOldData, NoDestructor, Protected, HasGetValueTypeHash)
	class ULiquidContainerComponent* LiquidContainer;                                           // 0x0828(0x0008) (Edit, ExportObject, ZeroConstructor, InstancedReference, IsPlainOldData, NoDestructor, Protected, HasGetValueTypeHash)
	unsigned char                                      UnknownData_7SEW[0x1];                                     // 0x0870(0x0001) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	unsigned char                                      UnknownData_NP3V[0x76];                                    // 0x0872(0x0076) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	class UParticleSystemComponent* BucketContentsEffect;                                      // 0x08E8(0x0008) (ExportObject, ZeroConstructor, InstancedReference, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	unsigned char                                      UnknownData_ZK3R[0x10];                                    // 0x08F0(0x0010) MISSED OFFSET (PADDING)
};

struct AProjectileWeapon {
	char pad[0x7d0]; // 0
	FProjectileWeaponParameters WeaponParameters; // 0x7d0

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

struct AShip
{
	char pad1[0x0518];
	bool                                               EmissaryFlagActive;                                        // 0x0FBA(0x0001) (Net, ZeroConstructor, IsPlainOldData, NoDestructor)

	FVector GetCurrentVelocity() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.Ship.GetCurrentVelocity");
		FVector params;
		ProcessEvent(this, fn, &params);
		return params;
	}
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

struct AAthenaGameState {
	char pad[0x05B8];
	struct AWindService* WindService; // 0x5b8(0x08)
	struct APlayerManagerService* PlayerManagerService; // 0x5c0(0x08)
	struct AShipService* ShipService; // 0x5c8(0x08)
	struct AWatercraftService* WatercraftService; // 0x5d0(0x08)
	struct ATimeService* TimeService; // 0x5d8(0x08)
	struct UHealthCustomizationService* HealthService; // 0x5e0(0x08)
	struct UCustomWeatherService* CustomWeatherService; // 0x5e8(0x08)
	struct AFFTWaterService* WaterService; // 0x5f0(0x08)
	struct AStormService* StormService; // 0x5f8(0x08)
	struct ACrewService* CrewService; // 0x600(0x08)
	struct AContestZoneService* ContestZoneService; // 0x608(0x08)
	struct AContestRowboatsService* ContestRowboatsService; // 0x610(0x08)
	struct AIslandService* IslandService; // 0x618(0x08)
	struct ANPCService* NPCService; // 0x620(0x08)
	struct ASkellyFortService* SkellyFortService; // 0x628(0x08)
	struct AAIDioramaService* AIDioramaService; // 0x630(0x08)
	struct AAshenLordEncounterService* AshenLordEncounterService; // 0x638(0x08)
	struct AAggressiveGhostShipsEncounterService* AggressiveGhostShipsEncounterService; // 0x640(0x08)
	struct ATallTaleService* TallTaleService; // 0x648(0x08)
	struct AAIShipObstacleService* AIShipObstacleService; // 0x650(0x08)
	struct AAIShipService* AIShipService; // 0x658(0x08)
	struct AAITargetService* AITargetService; // 0x660(0x08)
	struct UShipLiveryCatalogueService* ShipLiveryCatalogueService; // 0x668(0x08)
	struct AContestManagerService* ContestManagerService; // 0x670(0x08)
	struct ADrawDebugService* DrawDebugService; // 0x678(0x08)
	struct AWorldEventZoneService* WorldEventZoneService; // 0x680(0x08)
	struct UWorldResourceRegistry* WorldResourceRegistry; // 0x688(0x08)
	struct AKrakenService* KrakenService; // 0x690(0x08)
	struct UPlayerNameService* PlayerNameService; // 0x698(0x08)
	struct ATinySharkService* TinySharkService; // 0x6a0(0x08)
	struct AProjectileService* ProjectileService; // 0x6a8(0x08)
	struct ULaunchableProjectileService* LaunchableProjectileService; // 0x6b0(0x08)
	struct UServerNotificationsService* ServerNotificationsService; // 0x6b8(0x08)
	struct AAIManagerService* AIManagerService; // 0x6c0(0x08)
	struct AAIEncounterService* AIEncounterService; // 0x6c8(0x08)
	struct AAIEncounterGenerationService* AIEncounterGenerationService; // 0x6d0(0x08)
	struct UEncounterService* EncounterService; // 0x6d8(0x08)
	struct UGameEventSchedulerService* GameEventSchedulerService; // 0x6e0(0x08)
	struct UHideoutService* HideoutService; // 0x6e8(0x08)
	struct UAthenaStreamedLevelService* StreamedLevelService; // 0x6f0(0x08)
	struct ULocationProviderService* LocationProviderService; // 0x6f8(0x08)
	struct AHoleService* HoleService; // 0x700(0x08)
	struct APlayerBuriedItemService* PlayerBuriedItemService; // 0x708(0x08)
	struct ULoadoutService* LoadoutService; // 0x710(0x08)
	struct UOcclusionService* OcclusionService; // 0x718(0x08)
	struct UPetsService* PetsService; // 0x720(0x08)
	struct UAthenaAITeamsService* AthenaAITeamsService; // 0x728(0x08)
	struct AAllianceService* AllianceService; // 0x730(0x08)
	struct UMaterialAccessibilityService* MaterialAccessibilityService; // 0x738(0x08)
	struct AReapersMarkService* ReapersMarkService; // 0x740(0x08)
	struct AEmissaryLevelService* EmissaryLevelService; // 0x748(0x08)
	struct ACampaignService* CampaignService; // 0x750(0x08)
	struct AFlamesOfFateSettingsService* FlamesOfFateSettingsService; // 0x758(0x08)
	struct AServiceStatusNotificationsService* ServiceStatusNotificationsService; // 0x760(0x08)
	struct UMigrationService* MigrationService; // 0x768(0x08)
	struct AShroudBreakerService* ShroudBreakerService; // 0x770(0x08)
	struct UServerUpdateReportingService* ServerUpdateReportingService; // 0x778(0x08)
	struct AGenericMarkerService* GenericMarkerService; // 0x780(0x08)
	struct AMechanismsService* MechanismsService; // 0x788(0x08)
	struct UMerchantContractsService* MerchantContractsService; // 0x790(0x08)
	struct UShipFactory* ShipFactory; // 0x798(0x08)
	struct URewindPhysicsService* RewindPhysicsService; // 0x7a0(0x08)
	struct UNotificationMessagesDataAsset* NotificationMessagesDataAsset; // 0x7a8(0x08)
	struct AProjectileCooldownService* ProjectileCooldownService; // 0x7b0(0x08)
	struct UIslandReservationService* IslandReservationService; // 0x7b8(0x08)
	struct APortalService* PortalService; // 0x7c0(0x08)
	struct UMeshMemoryConstraintService* MeshMemoryConstraintService; // 0x7c8(0x08)
	struct ABootyStorageService* BootyStorageService; // 0x7d0(0x08)
	struct ASpireService* SpireService; // 0x7d8(0x08)
	struct UAirGivingService* AirGivingService; // 0x7e0(0x08)
	struct UCutsceneService* CutsceneService; // 0x7e8(0x08)
	struct ACargoRunService* CargoRunService; // 0x7f0(0x08)
	struct ACommodityDemandService* CommodityDemandService; // 0x7f8(0x08)
	struct ADebugTeleportationDestinationService* DebugTeleportationDestinationService; // 0x800(0x08)
	struct ASeasonProgressionUIService* SeasonProgressionUIService; // 0x808(0x08)
	struct UTunnelsOfTheDamnedService* TunnelsOfTheDamnedService; // 0x810(0x08)
	struct UWorldSequenceService* WorldSequenceService; // 0x818(0x08)
	struct UItemLifetimeManagerService* ItemLifetimeManagerService; // 0x820(0x08)
	struct USeaFortsService* SeaFortsService; // 0x828(0x08)
	struct UVolcanoService* VolcanoService; // 0x830(0x08)
	struct FString SubPlayMode; // 0xa88(0x10)
	struct UCustomVaultService* CustomVaultService; // 0xa98(0x08)
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

struct ULoadableComponent
{
	char pad1[0x00D0];
	float                                              LoadTime;                                                  // 0x00D0(0x0004) (Edit, BlueprintVisible, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              UnloadTime;                                                // 0x00D4(0x0004) (Edit, BlueprintVisible, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UClass* DefaultObjectToLoad;                                       // 0x00D8(0x0008) (Edit, BlueprintVisible, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, UObjectWrapper, HasGetValueTypeHash)
	unsigned char                                      UnknownData_GZLH[0x50];                                    // 0x00E0(0x0050) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	struct FTransform                                  UnloadingPoint;                                            // 0x0130(0x0030) (Edit, BlueprintVisible, DisableEditOnInstance, IsPlainOldData, NoDestructor, Protected)
	unsigned char                                      UnknownData_8A3T[0x18];                                    // 0x0160(0x0018) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	bool                                               AlwaysLoaded;                                              // 0x0188(0x0001) (Net, ZeroConstructor, IsPlainOldData, NoDestructor)
	unsigned char                                      UnknownData_FGJ0[0x57];                                    // 0x0189(0x0057) MISSED OFFSET (PADDING)
};

struct UBootyStorageSettings
{
	char pad1[0x0038];
	float                                              StoreHoldTime;                                             // 0x0038(0x0004) (Edit, ZeroConstructor, Config, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              RetrieveHoldTime;                                          // 0x003C(0x0004) (Edit, ZeroConstructor, Config, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              PickupPointSpawnDepth;                                     // 0x0040(0x0004) (Edit, ZeroConstructor, Config, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              PickupDismissDuration;                                     // 0x0044(0x0004) (Edit, ZeroConstructor, Config, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              PickupDismissDistanceCheck;                                // 0x0048(0x0004) (Edit, ZeroConstructor, Config, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	unsigned char                                      MaxStoragePerLocation;                                     // 0x004C(0x0001) (Edit, ZeroConstructor, Config, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	unsigned char                                      UnknownData_V0LN[0x3];                                     // 0x004D(0x0003) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	TArray<class UClass*>                              BlacklistedCategories;                                     // 0x0060(0x0010) (Edit, ZeroConstructor, Config, UObjectWrapper)
};

struct ABootyStorageService
{
	char pad1[0x0450];
	class UBootyStorageSettings* BootyStorageSettings;                                      // 0x0450(0x0008) (ZeroConstructor, Transient, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UBootyStorageSettingsAsset* BootyStorageSettingsAsset;                                 // 0x0458(0x0008) (ZeroConstructor, Transient, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	TArray<struct FCrewBootyStorage>                   Storage;                                                   // 0x0460(0x0010) (Net, ZeroConstructor, RepNotify)
	unsigned char                                      UnknownData_7TKK[0x8];                                     // 0x0750(0x0008) MISSED OFFSET (PADDING)
};

struct ACannon {
	char pad_4324[0x0528];
	struct USkeletalMeshComponent* BaseMeshComponent; // 0x528(0x08)
	struct UStaticMeshComponent* BarrelMeshComponent; // 0x530(0x08)
	struct UStaticMeshComponent* FuseMeshComponent; // 0x538(0x08)
	struct UReplicatedShipPartCustomizationComponent* CustomizationComponent; // 0x540(0x08)
	struct ULoadableComponent* LoadableComponent; // 0x548(0x08)
	struct ULoadingPointComponent* LoadingPointComponent; // 0x550(0x08)
	struct UChildActorComponent* CannonBarrelInteractionComponent; // 0x558(0x08)
	struct UFuseComponent* FuseComponent; // 0x560(0x08)
	struct FName CameraSocket; // 0x568(0x08)
	struct FName CameraInsideCannonSocket; // 0x570(0x08)
	struct FName LaunchSocket; // 0x578(0x08)
	struct FName TooltipSocket; // 0x580(0x08)
	struct FName AudioAimRTPCName; // 0x588(0x08)
	struct FName InsideCannonRTPCName; // 0x590(0x08)
	struct UClass* ProjectileClass; // 0x598(0x08)
	float TimePerFire; // 0x5a0(0x04)
	float ProjectileSpeed; // 0x5a4(0x04)
	float ProjectileGravityScale; // 0x5a8(0x04)
	struct FFloatRange PitchRange; // 0x5ac(0x10)
	struct FFloatRange YawRange; // 0x5bc(0x10)
	float PitchSpeed; // 0x5cc(0x04)
	float YawSpeed; // 0x5d0(0x04)
	char UnknownData_5D4[0x4]; // 0x5d4(0x04)
	struct UClass* CameraShake; // 0x5d8(0x08)
	float ShakeInnerRadius; // 0x5e0(0x04)
	float ShakeOuterRadius; // 0x5e4(0x04)
	float CannonFiredAINoiseRange; // 0x5e8(0x04)
	struct FName AINoiseTag; // 0x5ec(0x08)
	unsigned char                                      UnknownData_WIJI[0x4];                                     // 0x05F4(0x0004) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	char pad_956335424[0x18];
	unsigned char                                      UnknownData_YCZ3[0x20];                                    // 0x05F4(0x0020) FIX WRONG TYPE SIZE OF PREVIOUS PROPERTY
	char pad_9335443224[0x18];
	unsigned char                                      UnknownData_BEDV[0x20];                                    // 0x0630(0x0020) FIX WRONG TYPE SIZE OF PREVIOUS PROPERTY
	float DefaultFOV; // 0x668(0x04)
	float AimFOV; // 0x66c(0x04)
	float IntoAimBlendSpeed; // 0x670(0x04)
	float OutOfAimBlendSpeed; // 0x674(0x04)
	struct UWwiseEvent* FireSfx; // 0x678(0x08)
	struct UWwiseEvent* DryFireSfx; // 0x680(0x08)
	struct UWwiseEvent* LoadingSfx_Play; // 0x688(0x08)
	struct UWwiseEvent* LoadingSfx_Stop; // 0x690(0x08)
	struct UWwiseEvent* UnloadingSfx_Play; // 0x698(0x08)
	struct UWwiseEvent* UnloadingSfx_Stop; // 0x6a0(0x08)
	struct UWwiseEvent* LoadedPlayerSfx; // 0x6a8(0x08)
	struct UWwiseEvent* UnloadedPlayerSfx; // 0x6b0(0x08)
	struct UWwiseEvent* FiredPlayerSfx; // 0x6b8(0x08)
	struct UWwiseObjectPoolWrapper* SfxPool; // 0x6c0(0x08)
	struct UWwiseEvent* StartPitchMovement; // 0x6c8(0x08)
	struct UWwiseEvent* StopPitchMovement; // 0x6d0(0x08)
	struct UWwiseEvent* StartYawMovement; // 0x6d8(0x08)
	struct UWwiseEvent* StopYawMovement; // 0x6e0(0x08)
	struct UWwiseEvent* StopMovementAtEnd; // 0x6e8(0x08)
	struct UWwiseObjectPoolWrapper* SfxMovementPool; // 0x6f0(0x08)
	struct UObject* FuseVfxFirstPerson; // 0x6f8(0x08)
	struct UObject* FuseVfxThirdPerson; // 0x700(0x08)
	struct UObject* MuzzleFlashVfxFirstPerson; // 0x708(0x08)
	struct UObject* MuzzleFlashVfxThirdPerson; // 0x710(0x08)
	struct FName FuseSocketName; // 0x718(0x08)
	struct FName BarrelSocketName; // 0x720(0x08)
	struct UClass* RadialCategoryFilter; // 0x728(0x08)
	struct UClass* DefaultLoadedItemDesc; // 0x730(0x08)
	float ClientRotationBlendTime; // 0x738(0x04)
	char UnknownData_73C[0x4]; // 0x73c(0x04)
	struct AItemInfo* LoadedItemInfo; // 0x740(0x08)
	unsigned char UnknownData_LECI[0xc]; // 0x748(0x0c)
	float ServerPitch; // 0x754(0x04)
	float ServerYaw; // 0x758(0x04)


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

	bool IsReadyToReload() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.Cannon.IsReadyToReload");
		bool is_ready = true;
		ProcessEvent(this, fn, &is_ready);
		return is_ready;
	}

	void Reload() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.Cannon.Reload");
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

struct UItemDesc {
	char pad[0x0028];
	FString* Title; // 0x0028(0x38)
};

struct AItemInfo {
	char pad[0x0430];
	UItemDesc* Desc; // 0x0430(0x08)
};

struct FStorageContainerNode {
	struct UClass* ItemDesc; // 0x00(0x08)
	int32_t NumItems; // 0x08(0x04)
	char UnknownData_C[0x4]; // 0x0c(0x04)
};

struct AHarpoonLauncher {
	char pad[0xB04];
	FRotator rotation; // 0xB04
	// ROTATION OFFSET FOUND USING RECLASS.NET: https://www.unknowncheats.me/forum/sea-of-thieves/470590-reclass-net-plugin.html

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

class UActorComponent
{
	unsigned char                                      UnknownData_GRZN[0x8];                                     // 0x0028(0x0008) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	TArray<struct FName>                               ComponentTags;                                             // 0x0080(0x0010) (Edit, BlueprintVisible, ZeroConstructor)
	TArray<struct FSimpleMemberReference>              UCSModifiedProperties;                                     // 0x0090(0x0010) (ZeroConstructor)
	unsigned char                                      UnknownData_1HIB[0x10];                                    // 0x00A0(0x0010) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	TArray<class UAssetUserData*>                      AssetUserData;                                             // 0x00B0(0x0010) (ExportObject, ZeroConstructor, ContainsInstancedReference, Protected)
	unsigned char                                      UnknownData_2I5J : 3;                                      // 0x00C0(0x0001) BIT_FIELD (PADDING)
	unsigned char                                      bReplicates : 1;                                           // 0x00C0(0x0001) BIT_FIELD (Edit, BlueprintVisible, BlueprintReadOnly, Net, DisableEditOnInstance, NoDestructor)
	unsigned char                                      bNetAddressable : 1;                                       // 0x00C0(0x0001) BIT_FIELD (NoDestructor, Protected)
	unsigned char                                      UnknownData_G3LM : 3;                                      // 0x00C0(0x0001) BIT_FIELD (PADDING)
	unsigned char                                      UnknownData_UWFN : 6;                                      // 0x00C1(0x0001) BIT_FIELD (PADDING)
	unsigned char                                      bCreatedByConstructionScript : 1;                          // 0x00C1(0x0001) BIT_FIELD (Deprecated, NoDestructor)
	unsigned char                                      bInstanceComponent : 1;                                    // 0x00C1(0x0001) BIT_FIELD (Deprecated, NoDestructor)
	unsigned char                                      bAutoActivate : 1;                                         // 0x00C2(0x0001) BIT_FIELD (Edit, BlueprintVisible, BlueprintReadOnly, NoDestructor)
	unsigned char                                      bIsActive : 1;                                             // 0x00C2(0x0001) BIT_FIELD (Net, Transient, RepNotify, NoDestructor)
	unsigned char                                      bEditableWhenInherited : 1;                                // 0x00C2(0x0001) BIT_FIELD (Edit, DisableEditOnInstance, NoDestructor)
	unsigned char                                      UnknownData_4IP7 : 5;                                      // 0x00C2(0x0001) BIT_FIELD (PADDING)
	unsigned char                                      UnknownData_66RP : 3;                                      // 0x00C3(0x0001) BIT_FIELD (PADDING)
	unsigned char                                      bNeedsLoadForClient : 1;                                   // 0x00C3(0x0001) BIT_FIELD (Edit, NoDestructor)
	unsigned char                                      bNeedsLoadForServer : 1;                                   // 0x00C3(0x0001) BIT_FIELD (Edit, NoDestructor)
	unsigned char                                      UnknownData_0ZMF[0x2];                                     // 0x00C6(0x0002) MISSED OFFSET (PADDING)
};

struct UDrunkennessComponentPublicData
{
	char pad1[0x0028];
	TArray<struct FDrunkennessSetupData>               DrunkennessSetupData;                                      // 0x0028(0x0010) (Edit, ZeroConstructor)
	float                                              VomitingThresholdWhenGettingDrunker;                       // 0x0038(0x0004) (Edit, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              VomitingThresholdWhenSobering;                             // 0x003C(0x0004) (Edit, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              MinTimeBetweenVomitSpews;                                  // 0x0040(0x0004) (Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              MaxTimeBetweenVomitSpews;                                  // 0x0044(0x0004) (Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              MinVomitSpewDuration;                                      // 0x0048(0x0004) (Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              MaxVomitSpewDuration;                                      // 0x004C(0x0004) (Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              WaterSplashSoberingAmount;                                 // 0x0050(0x0004) (Edit, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              WaterSplashSoberingRate;                                   // 0x0054(0x0004) (Edit, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UCurveFloat* DrunkennessRemappingCurveForScreenVfx;                     // 0x0058(0x0008) (Edit, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UCurveFloat* DrunkennessRemappingCurveForStaggering;                    // 0x0060(0x0008) (Edit, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              CameraRollAmp;                                             // 0x0068(0x0004) (Edit, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              CameraRollAccel;                                           // 0x006C(0x0004) (Edit, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWwiseObjectPoolWrapper* DrunkennessComponentPool;                                  // 0x0070(0x0008) (Edit, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWwiseEvent* StartDrunkennessSfx;                                       // 0x0078(0x0008) (Edit, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWwiseEvent* StopDrunkennessSfx;                                        // 0x0080(0x0008) (Edit, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWwiseEvent* StartDrunkennessSfxRemotePlayer;                           // 0x0088(0x0008) (Edit, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	class UWwiseEvent* StopDrunkennessSfxRemotePlayer;                            // 0x0090(0x0008) (Edit, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	struct FName                                       PlayerDrunkennessAmountRtpc;                               // 0x0098(0x0008) (Edit, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	struct FName                                       RemotePlayerDrunkennessAmountRtpc;                         // 0x00A0(0x0008) (Edit, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              MinDrunkennessToToggleLocomotionAnimType;                  // 0x00A8(0x0004) (Edit, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	unsigned char                                      UnknownData_JG4X[0x4];                                     // 0x00AC(0x0004) MISSED OFFSET (PADDING)
};

struct UDrunkennessComponent
{
	char pad1[0x00D0];
	class UDrunkennessComponentPublicData* PublicData;                                                // 0x00D0(0x0008) (Edit, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	unsigned char                                      UnknownData_N5FF[0x140];                                   // 0x00D8(0x0140) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	float                                              TargetDrunkenness;                                    // 0x0218(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	float                                              CurrentDrunkenness;                                   // 0x0220(0x0008) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	float                                              RemainingAmountToSoberUpDueToWaterSplash;                  // 0x0228(0x0004) (Net, ZeroConstructor, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash)
	unsigned char                                      UnknownData_SJF3[0xC];                                     // 0x022C(0x000C) MISSED OFFSET (FIX SPACE BETWEEN PREVIOUS PROPERTY)
	struct FName                                       VomitVFXType;                                              // 0x0250(0x0008) (Edit, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
};

class ACharacter : public UObject {
public:

	char pad1[0x3C8];	//0x28 from inherit
	APlayerState* PlayerState;  // 0x03F0
	char pad2[0x10];
	AController* Controller; // 0x0408
	char pad3[0x38];
	USkeletalMeshComponent* Mesh; // 0x0448
	UCharacterMovementComponent* CharacterMovement; // 0x450
	char pad4[0x3D8];
	UWieldedItemComponent* WieldedItemComponent; // 0x0830
	char pad43[0x8];
	UInventoryManipulatorComponent* InventoryManipulatorComponent; // 0x0840
	char pad5[0x10];
	UHealthComponent* HealthComponent; // 0x0858(0x0008)
	char pad6[0x4F8];
	UDrunkennessComponent* DrunkennessComponent;  // 0x0D58(0x0008) (Edit, BlueprintVisible, ExportObject, BlueprintReadOnly, ZeroConstructor, EditConst, InstancedReference, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	char pad7[0x8];
	UDrowningComponent* DrowningComponent; // 0x0D68

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

	bool K2_SetActorLocation(FVector& NewLocation, bool bSweep, FHitResult& SweepHitResult, bool bTeleport)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.K2_SetActorLocation");
		struct
		{
			FVector NewLocation;
			bool bSweep = false;
			FHitResult SweepHitResult;
			bool bTeleport = false;
		} params;

		params.NewLocation = NewLocation;
		params.bSweep = bSweep;
		params.bTeleport = bTeleport;

		ProcessEvent(this, fn, &params);

		SweepHitResult = params.SweepHitResult;
	}

	bool BlueprintUpdateCamera(FVector& NewCameraLocation, FRotator& NewCameraRotation, float& NewCameraFOV)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Engine.PlayerCameraManager.BlueprintUpdateCamera");
		struct
		{
			FVector NewCameraLocation;
			FRotator NewCameraRotation;
			float NewCameraFOV;
		} params;

		params.NewCameraLocation = NewCameraLocation;
		params.NewCameraRotation = NewCameraRotation;
		params.NewCameraFOV = NewCameraFOV;

		ProcessEvent(this, fn, &params);

		NewCameraLocation = params.NewCameraLocation;
		NewCameraRotation = params.NewCameraRotation;
		NewCameraFOV = params.NewCameraFOV;
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

	FVector GetForwardVelocity() {
		static auto fn = UObject::FindObject<UFunction>("Function Engine.Actor.GetActorForwardVector");
		FVector ForwardVelocity;
		ProcessEvent(this, fn, &ForwardVelocity);
		return ForwardVelocity;
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

	bool IsLoading() {
		static auto fn = UObject::FindObject<UFunction>("Function AthenaLoadingScreen.AthenaLoadingScreenBlueprintFunctionLibrary.IsLoadingScreenVisible");
		bool isLoading = true;
		ProcessEvent(this, fn, &isLoading);
		return isLoading;
	}

	bool IsShipSinking()
	{
		static auto fn = UObject::FindObject<UFunction>("Function Athena.HullDamage.IsShipSinking");
		bool IsShipSinking = true;
		ProcessEvent(this, fn, &IsShipSinking);
		return IsShipSinking;
	}

	bool IsSinking()
	{
		static auto fn = UObject::FindObject<UFunction>("Function Athena.SinkingComponent.IsSinking");
		bool IsSinking = true;
		ProcessEvent(this, fn, &IsSinking);
		return IsSinking;
	}

	bool PlayerIsTeleporting()
	{
		static auto fn = UObject::FindObject<UFunction>("Function Athena.StreamingTelemetryBaseEvent.PlayerIsTeleporting");
		bool PlayerIsTeleporting = true;
		ProcessEvent(this, fn, &PlayerIsTeleporting);
		return PlayerIsTeleporting;
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

	inline bool isAmmoChest() {
		static auto obj = UObject::FindClass("Class Athena.AmmoChest");
		return IsA(obj);
	}

	inline bool isNewHole()
	{
		static auto obj = UObject::FindClass("BlueprintGeneratedClass BP_BaseInternalDamageZone.BP_BaseInternalDamageZone_C");
		return IsA(obj);
	}

	inline bool isKeg() {
		static auto obj = UObject::FindClass("");
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

	inline bool isMegalodon() {
		static auto obj = UObject::FindClass("Class Athena.TinyShark");
		return IsA(obj);
	}

	inline bool isMermaid() {
		static auto obj = UObject::FindClass("Class Athena.Mermaid");
		return IsA(obj);
	}

	inline bool isRowboat() {
		static auto obj = UObject::FindClass("Class Watercrafts.Rowboat");
		return IsA(obj);
	}

	inline bool isAnimal() {
		static auto obj = UObject::FindClass("Class AthenaAI.Fauna");
		return IsA(obj);
	}

	inline bool isEvent() {
		static auto obj = UObject::FindClass("Class Athena.GameplayEventSignal");
		return IsA(obj);
	}

	inline bool isSail() {
		static auto obj = UObject::FindClass("Class Athena.Sail");
		return IsA(obj);
	}

	inline bool isMast() {
		static auto obj = UObject::FindClass("Class Athena.Mast");
		return IsA(obj);
	}

	bool isWeapon() {
		static auto obj = UObject::FindClass("Class Athena.ProjectileWeapon");
		return IsA(obj);
	}

	bool isBucket() {
		static auto obj = UObject::FindClass("Class Athena.Bucket");
		return IsA(obj);
	}

	bool isSword() {
		static auto obj = UObject::FindClass("Class Athena.MeleeWeapon");
		return IsA(obj);
	}

	inline bool isBarrel() {
		static auto obj = UObject::FindClass("Class Athena.StorageContainer");
		return IsA(obj);
	}

	bool isBuriedTreasure() {
		static auto obj = UObject::FindClass("Class Athena.BuriedTreasureLocation");
		return IsA(obj);
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

	bool IsMastDown() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.Mast.IsMastFullyDamaged");
		bool ismastdown = false;
		ProcessEvent(this, fn, &ismastdown);
		return ismastdown;
	}

	void OpenFerryDoor() {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.GhostShipDoor.OpenForPlayer");
		ProcessEvent(this, fn, nullptr);
	}

	float GetTargetFOV(class AAthenaPlayerCharacter* Character) {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.FOVHandlerFunctions.GetTargetFOV");
		UFOVHandlerFunctions_GetTargetFOV_Params params;
		params.Character = Character;
		ProcessEvent(this, fn, &params);
		return params.ReturnValue;
	}

	void SetTime(int Hours)
	{
		static auto fn = UObject::FindObject<UFunction>("Function Athena.GameCustomizationService.SetTime");
		AGameCustomizationService_SetTime_Params params;
		params.Hours = Hours;
		ProcessEvent(this, fn, &params);
	}

	void SetTargetFOV(class AAthenaPlayerCharacter* Character, float TargetFOV) {
		static auto fn = UObject::FindObject<UFunction>("Function Athena.FOVHandlerFunctions.SetTargetFOV");
		UFOVHandlerFunctions_SetTargetFOV_Params params;
		params.Character = Character;
		params.TargetFOV = TargetFOV;
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
	float                                              ClampYawRange;                                             // 0x0238(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
	float                                              ClampYawRate;                                              // 0x023C(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
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
	char pad[0x07C0];
	struct UMeleeWeaponDataAsset* DataAsset; //0x07C0
};

struct AMapTable
{
	char pad[0x04E0];
	TArray<struct FVector2D> MapPins; // 0x04E0
};

#ifdef _MSC_VER
#pragma pack(pop)
#endif
struct Ray
{
    Vector3 origin;
    Vector3 direction;
};

enum BodyPart
{
    LOWERLEG_LEFT,
    LOWERLEG_RIGHT,
    UPPERLEG_LEFT,
    UPPERLEG_RIGHT,
    STOMACH,
    CHEST,
    UPPERARM_LEFT,
    UPPERARM_RIGHT,
    LOWERARM_LEFT,
    LOWERARM_RIGHT,
    HEAD
};

struct PlayerAdapter
{
    int pad_0[3];
    void* Player; // 0x10
};

enum WeaponCategory
{
    Pistol,
    AssaultRifle,
    SubmachineGun,
    Shotgun,
    SniperRifle,
    Melee,
    Grenade,
    Utility
};

struct HitData
{
    bool hitCharacter;
    bool traced;
    Vector3 hitWorldPos;
    int victimID;
    BodyPart hitBodyPart;
    Vector3 hitLocalPos;
    int hitAngle;
    Vector3 hitWorldNormal;
    int hitMaterialDef;
};

struct Character
{
    void* character;
    int id;
};

enum ChatMessageType
{
    PUBLIC_CHAT,
    TEAM_CHAT,
    RADIO,
    NOTIFICATION,
    LOCAL_NOTIFICATION,
    BROADCAST,
    FRIENDS,
    PARTY,
    CLAN
};

struct Enemy
{
    void* Character = nullptr;
    void* Player = nullptr;
    std::string Name = "";
    std::string Clan = "";
    int team = -1;
    bool local = false;
};
std::vector<Enemy> EnemyList;

struct TransformData
{
    // Pads may be wrong :(
    Vector3 pos;
    int pad_0[2];
    Vector3 velocity;
    int pad_1[3];
    Vector2 rotation;
};


struct ESPCfg
{
    bool snapline = 0;
    ImVec4 snaplineColor = ImColor(255,255,255);
    bool bone = 0;
    ImVec4 boneColor = ImColor(255,255,255);
    bool box = 0;
    ImVec4 boxColor = ImColor(255,255,255);
    bool healthesp = 0;
    bool healthNumber = 0;
    bool name = 0;
    bool distance;
    ImVec4 nameColor = ImColor(255,255,255);
    bool weapon = 0;
    ImVec4 weaponColor = ImColor(255,255,255);
};
struct BurstData
{
    Ray currentRay;
    void* weaponEgid;
    int damagePercent;
    bool trace;
    float maxRange;
};

struct AimbotCfg
{
    bool aimbot = 0;
    bool visCheck = 0;
    BodyPart aimBone = STOMACH;
    bool aimbotSmooth = 0;
    float smoothAmount = 1.0f;
    bool fovCheck = 0;
    float fovValue = 0.0f;
    bool drawFov = 0;
    bool onShoot = 0;
    bool triggerbot = 0;
};

struct CustomWeapon
{
    void* EGID = 0;
    int liveId = -1;
    int weaponDefId = -1;
};

struct Color
{
    float r;
    float g;
    float b;
    float a;
};

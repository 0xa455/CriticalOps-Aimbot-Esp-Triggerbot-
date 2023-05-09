#include GameTypes.h
ESPCfg invisibleCfg, espcfg;
AimbotCfg pistolCfg, smgCfg, arCFg, shotgunCfg, sniperCfg;

void* TouchControls = nullptr;
void* pSys = nullptr;

void *getTransform(void *character) {
    if (character) {
        return *(void **) ((uint64_t) character + string2Offset(OBFUSCATE("0x70")));
    }
    return nullptr;
}

int get_CharacterTeam(void* character)
{
    int team = std::stoi(OBFUSCATE("-1"));
    if (character != nullptr && get_IsInitialized(character))
    {
        void* player = *(void**)((uint64_t)character + string2Offset(OBFUSCATE("0x90")));
        if (player)
        {
            PlayerAdapter playerAdapter;
            PlayerAdapter_CTOR(&playerAdapter, player);
            if (&playerAdapter != nullptr && player != nullptr && *(uint64_t*)((uint64_t) player + 0x118) != 0 )
            {
                team = getTeamIndex(&playerAdapter);
            }
        }
    }

    return team;
}

std::string get_PlayerUsername(void* player){
    std::string username = OBFUSCATE("");
    std::string clantag = OBFUSCATE("");
    if(player != nullptr){
        PlayerAdapter* playerAdapter = new PlayerAdapter;
        playerAdapter->Player = player;
        username = get_Username(playerAdapter)->getString();
        clantag = get_ClanTag(playerAdapter)->getString();
        if(clantag != std::string(OBFUSCATE(""))){
            username = std::string(OBFUSCATE("[")) + clantag + std::string(OBFUSCATE("]")) + std::string(OBFUSCATE(" ")) +  username;
        }
        delete playerAdapter;
    }
    return username;
}

Vector3 getBonePosition(void *character, int bone) {
    void *hitSphere = NULL;
    void *transform = NULL;
    Vector3 bonePos = Vector3(0, 0, 0);
    if (character != nullptr) {
        void *curBone = get_CharacterBodyPart(character, bone);
        if (curBone != nullptr) { hitSphere = *(void **) ((uint64_t) curBone + 0x20); }
        if (hitSphere != nullptr) { transform = *(void **) ((uint64_t) hitSphere + 0x30); }
        if (transform != nullptr) {
            bonePos = get_Position(transform);
            return bonePos;
        }
        return Vector3(0, 0, 0);
    }
    return Vector3(0, 0, 0);
}

bool isInFov2(Vector2 rotation, Vector2 newAngle, AimbotCfg cfg) {
    if (!cfg.fovCheck) return true;

    Vector2 difference = newAngle - rotation;

    if (difference.Y > 180) difference.Y -= 360;
    if (difference.Y < -180) difference.Y += 360;

    if (sqrt(difference.X * difference.X + difference.Y * difference.Y) > cfg.fovValue) {
        return false;
    }
    return true;
}

void *getValidEnt3(AimbotCfg cfg, Vector2 rotation) {
    int id = getLocalId(pSys);
    if (id == 0) return nullptr;
    void *localPlayer = getPlayer(pSys, id);
    if (localPlayer == nullptr) return nullptr;
    int localTeam = localEnemy.team;
    float closestEntDist = 99999.0f;
    void *closestCharacter = nullptr;
    for (int i = 0; i < EnemyList.size(); i++){
        Enemy currentEnemy;
        currentEnemy = EnemyList[i];
        int curTeam = currentEnemy.team;
        int health = get_Health(currentEnemy.Character);
        bool canSet = false;
        Vector2 newAngle;

        if (cfg.aimbot && localEnemy.Character != nullptr) {
            if (get_Health(localEnemy.Character) > 0 && health > 0) {
                Vector3 localHead = getBonePosition(localEnemy.Character, 10);
                if (getIsCrouched(localEnemy.Character)) {
                    localHead = localHead - Vector3(0, 0.5, 0);
                }
                Vector3 enemyBone = getBonePosition(currentEnemy.Character, cfg.aimBone);
                Vector3 deltavec = enemyBone - localHead;
                float deltLength = sqrt(deltavec.X * deltavec.X + deltavec.Y * deltavec.Y +
                                        deltavec.Z * deltavec.Z);

                newAngle.X = -asin(deltavec.Y / deltLength) * (180.0 / PI);
                newAngle.Y = atan2(deltavec.X, deltavec.Z) * 180.0 / PI;
                if (isInFov2(rotation, newAngle, cfg)) {
                    if (localEnemy.Character && health > 0 && localTeam != curTeam && curTeam != -1) {
                        if (cfg.visCheck && get_Health(localEnemy.Character) > 0) {
                            if (isCharacterVisible(currentEnemy.Character, pSys)) {
                                canSet = true;
                            }
                        }

                        void *transform = getTransform(localEnemy.Character);
                        if (transform) {
                            Vector3 localPosition = get_Position(transform);
                            Vector3 currentCharacterPosition = get_Position(
                                    getTransform(currentEnemy.Character));
                            Vector3 currentEntDist = Vector3::Distance(localPosition,
                                                                       currentCharacterPosition);
                            if (Vector3::Magnitude(currentEntDist) < closestEntDist) {
                                if (cfg.visCheck && !canSet) continue;
                                closestEntDist = Vector3::Magnitude(currentEntDist);
                                closestCharacter = currentEnemy.Character;
                            }
                        }
                    }
                }
            }
        }
    }
    return closestCharacter;
}

bool isCharacterVisible(void *character, void *pSys) {
    void *localCharacter = get_LocalCharacter(pSys);
    if (localCharacter) {
        return !isHeadBehindWall(localCharacter, character);
    }
    return 0;
}

int getCurrentWeaponCategory(void *character) {
    void *characterData = *(void **) ((uint64_t) character + string2Offset(OBFUSCATE("0x98")));
    if (characterData) {
        void *m_wpn = *(void **) ((uint64_t) characterData + string2Offset(OBFUSCATE("0x80")));
        if (m_wpn) {
            return *(int *) ((uint64_t) m_wpn + string2Offset(OBFUSCATE("0x38")));
        }
    }
    return -1;
}

bool isCharacterShooting(void *character) {
    void *characterData = *(void **) ((uint64_t) character + string2Offset(OBFUSCATE("0x98")));
    if (characterData) {
        return *(bool *) ((uint64_t) characterData + string2Offset(OBFUSCATE("0x6C")));
    }
    return 0;
}

int currWeapon = -1;
void setRotation(void *character, Vector2 rotation) {
    std::lock_guard<std::mutex> guard(aimbot_mtx);
    Vector2 newAngle;
    Vector2 difference;
    difference.X = 0;
    difference.Y = 0;
    AimbotCfg cfg;
    if (localEnemy.Character != nullptr) {
        currWeapon = getCurrentWeaponCategory(localEnemy.Character);
        if (currWeapon != std::stoi(OBFUSCATE("-1"))) {
            switch (currWeapon) {
                case 0:
                    cfg = pistolCfg;
                    break;
                case 1:
                    cfg = arCFg;
                    break;
                case 2:
                    cfg = smgCfg;
                    break;
                case 3:
                    cfg = shotgunCfg;
                    break;
                case 4:
                    cfg = sniperCfg;
                    break;
            }
        }
    }
    void *closestEnt = nullptr;
    if (character != nullptr && localEnemy.Character != nullptr && get_IsInitialized(localEnemy.Character) && (pistolCfg.aimbot || shotgunCfg.aimbot || smgCfg.aimbot || arCFg.aimbot || sniperCfg.aimbot)) {
        closestEnt = getValidEnt3(cfg, rotation);
    }

    if (localEnemy.Character != nullptr && get_Health(localEnemy.Character) > 0 && closestEnt != nullptr) {
        Vector3 localHead = getBonePosition(localEnemy.Character, 10);
        if (localHead != Vector3(0, 0, 0) && getIsCrouched(localEnemy.Character)) {
            localHead = localHead - Vector3(0, 0.5, 0);
        }

        Vector3 enemyBone;
        if (aimPos == 0) {
            enemyBone = getBonePosition(closestEnt, HEAD);
        } else if (aimPos == 1) {
            enemyBone = getBonePosition(closestEnt, CHEST);
        } else if (aimPos == 2) {
            enemyBone = getBonePosition(closestEnt, STOMACH);
        }

        Vector3 deltavec = enemyBone - localHead;
        float deltLength = sqrt(deltavec.X * deltavec.X + deltavec.Y * deltavec.Y +
                                deltavec.Z * deltavec.Z);

        newAngle.X = -asin(deltavec.Y / deltLength) * (180.0 / PI);
        newAngle.Y = atan2(deltavec.X, deltavec.Z) * 180.0 / PI;

        if (character != nullptr) {
            if (cfg.aimbot && character == localEnemy.Character) {
                if (cfg.onShoot) {
                    if (isCharacterShooting(localEnemy.Character)) {
                        if (cfg.fovCheck) {
                            difference = isInFov(rotation, newAngle, cfg);
                        } else {
                            difference = newAngle - rotation;
                            cfg.fovValue = 360;
                        }
                    }
                } else {
                    if (cfg.fovCheck) {
                        difference = isInFov(rotation, newAngle, cfg);
                    } else {
                        difference = newAngle - rotation;
                        cfg.fovValue = 360;
                    }
                }

                if (cfg.aimbotSmooth) {
                    difference = difference / cfg.smoothAmount;
                }
            }
        }
    }


    if (cfg.triggerbot && closestEnt != nullptr && localEnemy.Character != nullptr && pSys != nullptr && get_Health(localEnemy.Character) > 0 && !get_Invulnerable(closestEnt)){
        int hitIndex = 0;
        void *camera = get_camera();
        if (camera != nullptr) {
            Ray ray = ScreenPointToRay(camera, Vector2(glWidth / 2, glHeight / 2), 2);

            if(closestEnt != nullptr){
                UpdateCharacterHitBuffer(pSys, closestEnt, ray, &hitIndex);
            }
            if (hitIndex && !shootControl) {
                shootControl = 1;
            }
        }
    }
    oSetRotation(character, rotation + difference);
}

bool isEnemyPresent(void* character)
{
    for(std::vector<Enemy>::iterator it = EnemyList.begin(); it != EnemyList.end(); it++){
        if((it)->Character == character){
            return true;
        }
    }
    return false;
}

void addEnemy(void* character)
{
    if(isEnemyPresent(character)){
        return;
    }

    Enemy enemy;
    enemy.Character = character;
    enemy.Player = get_Player(character);
    enemy.Name = get_PlayerUsername(enemy.Player);
    enemy.team = get_CharacterTeam(character);
    EnemyList.push_back(enemy);
}

void removeEnemy(void* character){
    for(int i = 0; i<EnemyList.size(); i++){
        if((EnemyList)[i].Character == character){
            EnemyList.erase(EnemyList.begin() + i);
            return;
        }
    }
}

void GameSystemUpdate(void *obj) {
    if (obj != nullptr) {
        if (pSys != obj) {
            std::lock_guard<std::mutex> guard(esp_mtx);
            // Stealing the object so we can call its member functions
            pSys = obj;
    }
    oldGameSystemUpdate(obj);
}

void GameSystemDestroy(void *obj) {
    oGameSystemDestroy(obj);
    for(int i = 0; i < EnemyList.size(); i++){
        removeEnemy(EnemyList[i].Character);
    }
    Enemy enemy;
    localEnemy = enemy;
    std::lock_guard<std::mutex> guard(esp_mtx);
    pSys = nullptr;
    localEnemy.Character = nullptr;
}

void TouchControlsUpdate(void *obj) {
    if (obj) {
        // Stealing the object so we can call its member functions
        TouchControls = obj;
    }
    return oTouchControlsUpdate(obj);
}

void TouchControlsDestroy(void *obj) {
    if (obj) {
        TouchControls = nullptr;
    }
    return oTouchControlsDestroy(obj);
}
  
void* CreateCharacter(void* obj, int id, Vector3 position, void* rotation, bool local){
    if(obj != nullptr){
        void* character = oldCreateCharacter(obj, id, position, rotation, local);
        addEnemy(character);
        return character;
    }
    return oldCreateCharacter(obj, id, position, rotation, local);
}

void* DestroyCharacter(void* obj, void* character, bool teamflip){
    if(obj != nullptr){
        removeEnemy(character);
    }
    return oldDestroyCharacter(obj, character, teamflip);
}
  
void CheckCharacterVisiblity(void *obj, bool *visibility) {
    oldCheckCharacterVisibility(obj, visibility);
    if (obj != nullptr) {
        *visibility = true;
    }
}

void ESP() {
    AimbotCfg cfg;

    std::lock_guard<std::mutex> guard(esp_mtx);
    auto background = ImGui::GetBackgroundDrawList();
    if (pSys == nullptr || !(esp || pistolCfg.aimbot || shotgunCfg.aimbot || smgCfg.aimbot || arCFg.aimbot || sniperCfg.aimbot)) {
        return;
    }

    int id = getLocalId(pSys);
    if (id == 0) {
        return;
    }

    if (pSys == nullptr) {
        return;
    }

    void *localPlayer = getPlayer(pSys, id);
    if (localPlayer == nullptr) {
        return;
    }

    auto cam = get_camera();
    if (cam == nullptr) {
        return;
    }

    for (int i = 0; i < EnemyList.size(); i++) {
        Enemy currentEnemy = EnemyList[i];
        void *currentCharacter = currentEnemy.Character;
        if (currentCharacter == nullptr) {
            continue;
        }
        void *currentPlayer = currentEnemy.Player;
        if (currentPlayer == nullptr) {
            continue;
        }

        if(localPlayer == currentEnemy.Player){
            localEnemy = currentEnemy;
        }
        int health = std::stoi(OBFUSCATE("0")), localTeam = std::stoi(OBFUSCATE("-1")), curTeam = std::stoi(OBFUSCATE("-1"));
        if(localEnemy.team != std::stoi(OBFUSCATE("-1")) && currentEnemy.team != std::stoi(OBFUSCATE("-1"))){
            health = get_Health(currentEnemy.Character);
            localTeam = localEnemy.team;
            curTeam = currentEnemy.team;
        }

        if (health <= std::stoi(OBFUSCATE("0")) || localTeam == curTeam || curTeam == std::stoi(OBFUSCATE("-1"))) {
            continue;
        }

        void *transform = getTransform(currentCharacter);
        void *localTransform = getTransform(localEnemy.Character);
        if (transform == nullptr || localTransform == nullptr) {
            continue;
        }

        Vector3 position = get_Position(transform);
        Vector3 transformPos = WorldToScreen(cam, position, 2);
        transformPos.Y = glHeight - transformPos.Y;
        Vector3 headPos = getBonePosition(currentCharacter, HEAD);
        Vector3 chestPos = getBonePosition(currentCharacter, CHEST);
        Vector3 wschestPos = WorldToScreen(cam, chestPos, 2);
        Vector3 wsheadPos = WorldToScreen(cam, headPos, 2);
        Vector3 aboveHead = headPos + Vector3(0, 0.2, 0);
        Vector3 headEstimate = position + Vector3(0, 1.48, 0);

        Vector3 wsAboveHead = WorldToScreen(cam, aboveHead, 2);
        Vector3 wsheadEstimate = WorldToScreen(cam, headEstimate, 2);

        wsAboveHead.Y = glHeight - wsAboveHead.Y;
        wsheadEstimate.Y = glHeight - wsheadEstimate.Y;

        float height = transformPos.Y - wsAboveHead.Y;
        float width = (transformPos.Y - wsheadEstimate.Y) / 2;

        Vector3 localPosition = get_Position(localTransform);
        Vector3 currentCharacterPosition = get_Position(transform);
        float currentEntDist = Vector3::Distance(localPosition, currentCharacterPosition);

        espcfg = invisibleCfg;

        if (esp) {
            if (espcfg.snapline && transformPos.Z > 0) {
                DrawLine(ImVec2(glWidth / 2, glHeight), ImVec2(transformPos.X, transformPos.Y), ImColor(espcfg.snaplineColor.x, espcfg.snaplineColor.y, espcfg.snaplineColor.z, (255 - currentEntDist * 5.0) / 255), 3, background);
            }

            if (espcfg.bone && transformPos.Z > 0 && currentCharacter != nullptr) {
                DrawBones(currentCharacter, LOWERLEG_LEFT, UPPERLEG_LEFT, espcfg, background);
                DrawBones(currentCharacter, LOWERLEG_RIGHT, UPPERLEG_RIGHT, espcfg, background);
                DrawBones(currentCharacter, UPPERLEG_LEFT, STOMACH, espcfg, background);
                DrawBones(currentCharacter, UPPERLEG_RIGHT, STOMACH, espcfg, background);
                DrawBones(currentCharacter, STOMACH, CHEST, espcfg, background);
                DrawBones(currentCharacter, LOWERARM_LEFT, UPPERARM_LEFT, espcfg, background);
                DrawBones(currentCharacter, LOWERARM_RIGHT, UPPERARM_RIGHT, espcfg, background);
                DrawBones(currentCharacter, UPPERARM_LEFT, CHEST, espcfg, background);
                DrawBones(currentCharacter, UPPERARM_RIGHT, CHEST, espcfg, background);
                Vector3 diff = wschestPos - wsheadPos;
                Vector3 neck = (chestPos + headPos) / 2;
                Vector3 wsneck = WorldToScreen(cam, neck, 2);
                wsneck.Y = glHeight - wsneck.Y;
                wschestPos.Y = glHeight - wschestPos.Y;
                wsheadPos.Y = glHeight - wsheadPos.Y;
                if (wschestPos.Z > 0 && wsneck.Z) {
                    DrawLine(ImVec2(wschestPos.X, wschestPos.Y), ImVec2(wsneck.X, wsneck.Y), ImColor(espcfg.boneColor.x, espcfg.boneColor.y, espcfg.boneColor.z), 3, background);
                }

                if (wsheadPos.Z > 0 && wschestPos.Z > 0) {
                    float radius = sqrt(diff.X * diff.X + diff.Y * diff.Y);
                    background->AddCircle(ImVec2(wsheadPos.X, wsheadPos.Y), radius / 2, IM_COL32(espcfg.boneColor.x * 255, espcfg.boneColor.y * 255, espcfg.boneColor.z * 255, 255), 0, 3.0f);
                }
            }

            if (espcfg.box && transformPos.Z > 0 && wsAboveHead.Z > 0) {
                DrawOutlinedBox2(wsAboveHead.X - width / 2, wsAboveHead.Y, width, height, ImVec4(espcfg.boxColor.x, espcfg.boxColor.y, espcfg.boxColor.z, 255), 3, background);
            }

            if (espcfg.healthesp && transformPos.Z > 0 && wsAboveHead.Z > 0) {
                DrawOutlinedFilledRect(wsAboveHead.X - width / 2 - 12, wsAboveHead.Y + height * (1 - (static_cast<float>(health) / 100.0f)), 3, height * (static_cast<float>(health) / 100.0f), HealthToColor(health), background);
            }

            if (espcfg.healthNumber && transformPos.Z > 0 && wsAboveHead.Z > 0) {
                if (health > 100) {
                    DrawText(ImVec2(wsAboveHead.X - width / 2 - 17, wsAboveHead.Y + height * (1 - static_cast<float>(health) / 100.0f) - 3), ImVec4(1, 1, 1, 255), std::to_string(health), espFont, background);
                }
            }

            if (espcfg.distance && transformPos.Z > 0) {
                DrawText(ImVec2(transformPos.X + width / 2, transformPos.Y - 12), ImVec4(1, 1, 1, 255), std::to_string(static_cast<int>(currentEntDist)) + "m", espFont, background);
            }

            if (espcfg.name && transformPos.Z > 0 && currentCharacter != nullptr) {
                void *player = get_Player(currentCharacter);
                if (player == nullptr)
                    continue;
                std::string username = get_PlayerUsername(player);
                float compensation = strlen(username.c_str()) * 4.0f;
                DrawText(ImVec2(wsheadPos.X - compensation, wsAboveHead.Y - 20), ImVec4(espcfg.nameColor.x, espcfg.nameColor.y, espcfg.nameColor.z, 255), username, espFont, background);
            }
        }
    }
    if (currWeapon != -1) {
        switch (currWeapon) {
            case 0:
                cfg = pistolCfg;
                break;
            case 1:
                cfg = arCFg;
                break;
            case 2:
                cfg = smgCfg;
                break;
            case 3:
                cfg = shotgunCfg;
                break;
            case 4:
                cfg = sniperCfg;
                break;
        }

        if (cfg.drawFov && pSys != nullptr) {
            float worldFov = get_cameraFov(pSys);
            float radius =tan(cfg.fovValue * (PI / 180) / 2) / tan(90 / 2) *glWidth / 2;
            auto background = ImGui::GetBackgroundDrawList();
            float correction = 0;
            if ((worldFov - 90) < 0) {
                correction = worldFov - 90;
            }
            background->AddCircle(ImVec2(glWidth / 2, glHeight / 2), (radius) * PI - correction / PI * 2, IM_COL32(255, 255, 255, 255), 0, 3.0f);
        }
    }
}
  
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    eglQuerySurface(dpy, surface, EGL_WIDTH, &glWidth);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &glHeight);

    ESP()

    ImGui::EndFrame();
    ImGui::Render();
    glViewport(0, 0, (int) get_Width(), (int) get_Height());
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}

void Pointers() {
    // couldnt be asked to remove all of the rest which arent used 
    get_Width = (int (*)()) get_absolute_address(string2Offset(OBFUSCATE("0x1669CDC"))); // screen.get_Width
    get_Height = (int (*)()) get_absolute_address(string2Offset(OBFUSCATE("0x1669D04"))); // screen.get_Height
    getAllCharacters = (monoList<void **> *(*)(void *)) get_absolute_address(string2Offset(OBFUSCATE("0x14E4010"))); // get_AllCharacters
    getLocalId = (int (*)(void *)) get_absolute_address(string2Offset(OBFUSCATE("0x14D7FF0"))); // get_LocalId
    getPlayer = (void *(*)(void *, int)) get_absolute_address(string2Offset(OBFUSCATE("0x14E7300"))); // GameSystem.GetPlayer
    getLocalPlayer = (void *(*)(void *)) get_absolute_address(string2Offset(OBFUSCATE("0x14D79D4"))); // GameSystem.get_LocalPlayer
    getCharacterCount = (int (*)(void *)) get_absolute_address(string2Offset(OBFUSCATE("0x14E4020"))); // GameSystem.get_CharacterCount
    get_Health = (int (*)(void *)) get_absolute_address(string2Offset(OBFUSCATE("0x11846B8"))); // get_Health
    get_Player = (void *(*)(void *)) get_absolute_address(string2Offset(OBFUSCATE("0x11846A0"))); // Gameplay.Character.get_Player
    get_IsInitialized = (bool (*)(void *)) get_absolute_address(string2Offset(OBFUSCATE("0x1184528"))); // Gameplay.Character.get_IsInitialized
    get_Position = (Vector3(*)(void *)) get_absolute_address(string2Offset(OBFUSCATE("0x1972FE8"))); // Transform.get_position
    get_camera = (void *(*)()) get_absolute_address(string2Offset(OBFUSCATE("0x16719A0"))); // camera.get_main
    WorldToScreen = (Vector3(*)(void *, Vector3, int)) get_absolute_address(string2Offset(OBFUSCATE("0x1670DA4"))); // WorldToScreenPoint
    get_CharacterBodyPart = (void *(*)(void *, int)) get_absolute_address(string2Offset(OBFUSCATE("0x1184714"))); // Character.GetBodyPart
    get_LocalCharacter = (void *(*)(void *)) get_absolute_address(string2Offset(OBFUSCATE("0x14DA66C"))); // get_LocalCharacter
    isHeadBehindWall = (bool (*)(void *, void *)) get_absolute_address(string2Offset(OBFUSCATE("0x1332334"))); // IsHeadBehindWall
    get_FovWorld = (float (*)(void *)) get_absolute_address(string2Offset(OBFUSCATE("0x14D5AF0"))); // get_FovWorld
    getIsCrouched = (bool (*)(void *)) get_absolute_address(string2Offset(OBFUSCATE("0x1184540"))); // ISCrouched
    ScreenPointToRay = (Ray(*)(void *, Vector2, int)) get_absolute_address(string2Offset(OBFUSCATE("0x167147C"))); // ScreenPointToRay
    onInputButtons = (void (*)(void *, int, bool)) get_absolute_address(string2Offset(OBFUSCATE("0x1965114"))); // OnInputButton
    TraceShot = (void *(*)(void *, void *, Ray)) get_absolute_address(string2Offset(OBFUSCATE("0x14EB3C0"))); // TraceShot
    UpdateCharacterHitBuffer = (void (*)(void *, void *, Ray, int *))get_absolute_address(string2Offset(OBFUSCATE("0x14ED9D4"))); // UpdateCharacterHitBuffer
    get_Username = (monoString *(*)(PlayerAdapter *))get_absolute_address(string2Offset(OBFUSCATE("0x12D7B3C"))); // PlayerAdapter.get_Username
    get_ClanTag = (monoString *(*)(PlayerAdapter *))get_absolute_address(string2Offset(OBFUSCATE("0x12D7E8C"))); // PlayerAdapter.get_ClanTag
    CreateMessage = (void *(*)(monoString *, ChatMessageType, bool))get_absolute_address(string2Offset(OBFUSCATE("0x1639768"))); // RequestSendIngameChatMessage.Create
    SendMessage = (void (*)(void*))get_absolute_address(string2Offset(OBFUSCATE("0x183C114"))); // InGameChatMenu.SendMessage
    get_Invulnerable = (bool (*)(void*))get_absolute_address(string2Offset(OBFUSCATE("0x124E9AC"))); // Character.Invulnerable
    PlayerAdapter_CTOR = (void (*)(void*, void*))get_absolute_address(string2Offset(OBFUSCATE("0x12D79C4"))); // PlayerAdapter.PlayerAdapter
    Clear = (void (*)(void*))get_absolute_address(string2Offset(OBFUSCATE("0x1238AB0"))); // ShootData.Clear
    AddHit = (void (*)(void*, HitData))get_absolute_address(string2Offset(OBFUSCATE("0x1238B44"))); // ShootData.AddHit
    get_ID = (int (*)(void*))get_absolute_address(string2Offset(OBFUSCATE("0x1184404"))); // Gameplay.Character.get_ID
    set_Position = (void(*)(void*, Vector3))get_absolute_address(string2Offset(OBFUSCATE("0x1972FE8"))); // Transform.set_Position
}

void Hooks() {
    HOOK("0x14E8E80", GameSystemUpdate, oldGameSystemUpdate); // GameSystem.Update
    HOOK("0x14E4FAC", GameSystemDestroy, oGameSystemDestroy); // GameSystem.Destroy
    HOOK("0x19622F4", TouchControlsUpdate, oTouchControlsUpdate); // touchcontrols.update
    HOOK("0x1961C0C", TouchControlsDestroy, oTouchControlsDestroy); // touchcontrols.ondestroy
    HOOK("0x118693C", CheckCharacterVisiblity, oldCheckCharacterVisibility);
    HOOK("0x14E6AB8", CreateCharacter, oldCreateCharacter); // Chaarcter.Create
    HOOK("0x14E98F8", DestroyCharacter, oldDestroyCharacter); // Character.Destroy
    HOOK("0x118693C", CheckCharacterVisiblity, oldCheckCharacterVisibility); // Makes bones not T pose 
}

void *hack_thread(void *arg) {
    do {
        sleep(1);
        auto maps = KittyMemory::getMapsByName(OBFUSCATE("split_config.arm64_v8a.apk"));
        for (std::vector<ProcMap>::iterator it = maps.begin(); it != maps.end(); ++it) {
        
            // Getting the base address by pattern scanning through the split apk for the first bytes of the libil2cpp.so
            auto address = KittyScanner::findHexFirst(it->startAddress, it->endAddress,
                                                      OBFUSCATE("7F 45 4C 46 02 01 01 00 00 00 00 00 00 00 00 00 03 00 B7 00 01 00 00 00 B0 05 67 00 00 00 00 00 40 00 00 00 00 00 00 00 00 51 3B 02 00 00 00 00"),
                                                      OBFUSCATE("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"));
            if (address != 0) {
                libBaseAddress = address;
                libBaseEndAddress = it->endAddress;
                libSize = it->length;
            }
        }
    } while (libBaseAddress == 0);
    
    Pointers();
    Hooks();

    auto eglhandle = dlopen(OBFUSCATE("libunity.so"), RTLD_LAZY);
    auto eglSwapBuffers = dlsym(eglhandle, OBFUSCATE("eglSwapBuffers"));
    auto renderHandle = dlopen(OBFUSCATE("/system/lib64/libGLESv2.so"), RTLD_LAZY);
    DobbyHook((void *) eglSwapBuffers, (void *) hook_eglSwapBuffers, (void **) &old_eglSwapBuffers);

    return nullptr;
}

void *triggerbot_thread(void *arg) {
    while (true) {
        if (TouchControls && shootControl && localEnemy.Character && get_Health(localEnemy.Character)) {
            onInputButtons(TouchControls, 7, 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            onInputButtons(TouchControls, 7, 0);
            shootControl = 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return nullptr;
}

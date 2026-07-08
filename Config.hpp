#pragma once
#include <windows.h>
#include "offsets.hpp"
#include "Hotkeys.hpp"
#include "RuntimePaths.hpp"
#include <string>
#include <cctype>

namespace Config {

    inline std::string lastStatus = "";
    inline uint64_t lastStatusTime = 0;

    inline void SetStatus(const char* msg) {
        lastStatus = msg;
        lastStatusTime = GetTickCount64();
    }

    inline const char* GetDefaultConfigPath() {
        return RuntimePaths::DefaultConfigPath();
    }

    inline std::string GetConfigPathForName(const char* name) {
        std::string cfg = name && name[0] ? name : "default";
        for (char& c : cfg) {
            if (!(std::isalnum((unsigned char)c) || c == '_' || c == '-' || c == '.')) c = '_';
        }

        if (cfg == "default") return GetDefaultConfigPath();
        char tempPath[MAX_PATH];
        GetTempPathA(MAX_PATH, tempPath);
        return std::string(tempPath) + RuntimePaths::ConfigPrefix() + "_" + cfg + ".dat";
    }

    inline void SaveBool(HANDLE h, const char* key, bool val) {
        char buf[256];
        wsprintfA(buf, "%s=%d\r\n", key, val ? 1 : 0);
        DWORD w; WriteFile(h, buf, lstrlenA(buf), &w, NULL);
    }
    inline void SaveInt(HANDLE h, const char* key, int val) {
        char buf[256];
        wsprintfA(buf, "%s=%d\r\n", key, val);
        DWORD w; WriteFile(h, buf, lstrlenA(buf), &w, NULL);
    }
    inline void SaveFloat(HANDLE h, const char* key, float val) {
        char buf[256];
        int whole = (int)val;
        int frac = (int)((val - whole) * 100);
        if (frac < 0) frac = -frac;
        wsprintfA(buf, "%s=%d.%02d\r\n", key, whole, frac);
        DWORD w; WriteFile(h, buf, lstrlenA(buf), &w, NULL);
    }

    inline void SaveColor(HANDLE h, const char* key, const ImColor& col) {
        char buf[256];
        wsprintfA(buf, "%s=%d,%d,%d,%d\r\n", key,
            (int)(col.Value.x * 255.f), (int)(col.Value.y * 255.f),
            (int)(col.Value.z * 255.f), (int)(col.Value.w * 255.f));
        DWORD w; WriteFile(h, buf, lstrlenA(buf), &w, NULL);
    }

    inline bool Save() {
        HANDLE h = CreateFileA(GetDefaultConfigPath(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE) {
            SetStatus("Save failed: access denied");
            return false;
        }

        SaveInt(h, "Config.Version", 2);
        SaveBool(h, "ESP.Box", ESP::Box);
        SaveBool(h, "ESP.Enabled", ESP::ESPEnabled);
        SaveBool(h, "ESP.CornerBox", ESP::CornerBox);
        SaveBool(h, "ESP.FilledBox", ESP::FilledBox);
        SaveBool(h, "ESP.Name", ESP::Name);
        SaveBool(h, "ESP.Distance", ESP::Distance);
        SaveBool(h, "ESP.Skeleton", ESP::Skeleton);
        SaveBool(h, "ESP.HeadCircle", ESP::HeadCircle);
        SaveBool(h, "ESP.HealthBar", ESP::HealthBar);
        SaveBool(h, "ESP.Weapon", ESP::Weapon);
        SaveBool(h, "ESP.SnapLines", ESP::SnapLines);
        SaveBool(h, "ESP.OFFArrows", ESP::OFFArrows);
        SaveBool(h, "ESP.VisCheck", ESP::VisCheck);
        SaveInt(h, "ESP.VisMode", ESP::VisMode);
        SaveInt(h, "ESP.VisSamples", ESP::VisSamples);
        SaveInt(h, "ESP.VisHoldMs", ESP::VisHoldMs);
        SaveFloat(h, "ESP.OccluderDist", ESP::OccluderDist);
        SaveBool(h, "ESP.TeamID", ESP::TeamID);
        SaveBool(h, "ESP.hotbar_text", ESP::hotbar_text);
        SaveBool(h, "ESP.HotbarIcons", ESP::HotbarIcons);
        SaveBool(h, "ESP.RemoveWounded", ESP::RemoveWounded);
        SaveBool(h, "ESP.HighlightWounded", ESP::HighlightWounded);
        SaveBool(h, "ESP.RemoveSleepers", ESP::RemoveSleepers);
        SaveBool(h, "ESP.HighlightSleepers", ESP::HighlightSleepers);
        SaveBool(h, "ESP.RemoveTeam", ESP::RemoveTeam);
        SaveBool(h, "ESP.DrawTextBackground", ESP::DrawTextBackground);
        SaveBool(h, "ESP.SkeletonOutline", ESP::SkeletonOutline);
        SaveInt(h, "ESP.SkeletonThickness", ESP::SkeletonThickness);
        SaveBool(h, "ESP.BoxOutline", ESP::BoxOutline);
        SaveInt(h, "ESP.BoxThickness", ESP::BoxThickness);
        SaveBool(h, "ESP.SnaplineOutline", ESP::SnaplineOutline);
        SaveInt(h, "ESP.SnaplineThickness", ESP::SnaplineThickness);
        SaveBool(h, "ESP.Clothing", ESP::Clothing);
        SaveBool(h, "ESP.ItemList", ESP::ItemList);
        SaveBool(h, "ESP.AmmoBar", ESP::AmmoBar);
        SaveBool(h, "ESP.PlayerInventoryPanel", ESP::PlayerInventoryPanel);
        SaveBool(h, "ESP.MovementTrails", ESP::MovementTrails);
        SaveBool(h, "ESP.BulletTracers", ESP::BulletTracers);
        SaveBool(h, "ESP.ShowVisibility", ESP::ShowVisibility);
        SaveFloat(h, "ESP.TracerThickness", ESP::TracerThickness);
        SaveBool(h, "ESP.TracerOutline", ESP::TracerOutline);
        SaveFloat(h, "ESP.EspFontSize", ESP::EspFontSize);
        SaveInt(h, "ESP.DistanceBracketStyle", ESP::DistanceBracketStyle);
        SaveBool(h, "ESP.ReloadBar", ESP::ReloadBar);
        SaveFloat(h, "ESP.draw_distance", ESP::draw_distance);
        SaveBool(h, "ESP.ESPAdvanced", ESP::ESPAdvanced);
        SaveFloat(h, "ESP.BoxDist", ESP::BoxDist);
        SaveFloat(h, "ESP.SkeletonDist", ESP::SkeletonDist);
        SaveFloat(h, "ESP.NameDist", ESP::NameDist);
        SaveFloat(h, "ESP.WeaponDist", ESP::WeaponDist);
        SaveFloat(h, "ESP.HealthBarDist", ESP::HealthBarDist);
        SaveFloat(h, "ESP.SnaplineDist", ESP::SnaplineDist);
        SaveFloat(h, "ESP.HeadCircleDist", ESP::HeadCircleDist);
        SaveFloat(h, "ESP.OFFArrowDist", ESP::OFFArrowDist);
        SaveInt(h, "ESP.SnaplineMode", ESP::SnaplineMode);
        SaveBool(h, "ESP.VisGhost", ESP::VisGhost);
        SaveInt(h, "ESP.VisGhostMs", ESP::VisGhostMs);
        SaveColor(h, "ESP.color.Tracer", ESP::TracerColor);
        SaveColor(h, "ESP.color.Box", ESP::color::Box);
        SaveColor(h, "ESP.color.FilledBox", ESP::color::FilledBox);
        SaveColor(h, "ESP.color.Distance", ESP::color::Distance);
        SaveColor(h, "ESP.color.Snaplines", ESP::color::Snaplines);
        SaveColor(h, "ESP.color.HeadCircle", ESP::color::HeadCircle);
        SaveColor(h, "ESP.color.Weapon", ESP::color::Weapon);
        SaveColor(h, "ESP.color.Name", ESP::color::Name);
        SaveColor(h, "ESP.color.Skeleton", ESP::color::Skeleton);
        SaveColor(h, "ESP.color.Wounded", ESP::color::Wounded);
        SaveColor(h, "ESP.color.Sleepers", ESP::color::Sleepers);
        SaveColor(h, "ESP.color.Team", ESP::color::Team);
        SaveColor(h, "ESP.color.OFFArrow", ESP::color::OFFArrowColor);
        SaveColor(h, "ESP.color.Visible", ESP::color::Visible);
        SaveColor(h, "ESP.color.Invisible", ESP::color::Invisible);
        SaveColor(h, "ESP.color.SkeletonVisible", ESP::color::SkeletonVisible);
        SaveColor(h, "ESP.color.SkeletonInvisible", ESP::color::SkeletonInvisible);
        SaveColor(h, "ESP.color.HealthBar", ESP::color::HealthBar);
        SaveColor(h, "ESP.color.Ghost", ESP::color::Ghost);
        SaveColor(h, "ESP.color.Teammate", ESP::color::Teammate);

        SaveBool(h, "NPC.Enabled", NPC_ESP::Enabled);
        SaveBool(h, "NPC.ESPAdvanced", NPC_ESP::ESPAdvanced);
        SaveBool(h, "NPC.Box", NPC_ESP::Box);
        SaveBool(h, "NPC.Name", NPC_ESP::Name);
        SaveBool(h, "NPC.Distance", NPC_ESP::Distance);
        SaveBool(h, "NPC.HealthBar", NPC_ESP::HealthBar);
        SaveBool(h, "NPC.Skeleton", NPC_ESP::Skeleton);
        SaveBool(h, "NPC.HeldItem", NPC_ESP::HeldItem);
        SaveBool(h, "NPC.SnapLines", NPC_ESP::SnapLines);
        SaveInt(h, "NPC.SnaplineMode", NPC_ESP::SnaplineMode);
        SaveFloat(h, "NPC.draw_distance", NPC_ESP::draw_distance);
        SaveFloat(h, "NPC.BoxDist", NPC_ESP::NPCBoxDist);
        SaveFloat(h, "NPC.NameDist", NPC_ESP::NPCNameDist);
        SaveFloat(h, "NPC.DistTextDist", NPC_ESP::NPCDistTextDist);
        SaveFloat(h, "NPC.HPBarDist", NPC_ESP::NPCHealthBarDist);
        SaveFloat(h, "NPC.SkeletonDist", NPC_ESP::NPCSkeletonDist);
        SaveFloat(h, "NPC.HeldItemDist", NPC_ESP::NPCHeldItemDist);
        SaveFloat(h, "NPC.SnaplineDist", NPC_ESP::NPCSnaplineDist);
        SaveBool(h, "NPC.Scientists", NPC_ESP::Scientists);
        SaveBool(h, "NPC.TunnelDweller", NPC_ESP::TunnelDweller);
        SaveBool(h, "NPC.UnderwaterDweller", NPC_ESP::UnderwaterDweller);
        SaveColor(h, "NPC.color.Box", NPC_ESP::color::Box);
        SaveColor(h, "NPC.color.Name", NPC_ESP::color::Name);
        SaveColor(h, "NPC.color.Distance", NPC_ESP::color::Distance);
        SaveColor(h, "NPC.color.Skeleton", NPC_ESP::color::Skeleton);
        SaveColor(h, "NPC.color.Snaplines", NPC_ESP::color::Snaplines);

        SaveBool(h, "ANIMAL.Enabled", ANIMAL_ESP::Enabled);
        SaveBool(h, "ANIMAL.ESPAdvanced", ANIMAL_ESP::ESPAdvanced);
        SaveBool(h, "ANIMAL.Box", ANIMAL_ESP::Box);
        SaveBool(h, "ANIMAL.Name", ANIMAL_ESP::Name);
        SaveBool(h, "ANIMAL.Distance", ANIMAL_ESP::Distance);
        SaveBool(h, "ANIMAL.HealthBar", ANIMAL_ESP::HealthBar);
        SaveBool(h, "ANIMAL.SnapLines", ANIMAL_ESP::SnapLines);
        SaveInt(h, "ANIMAL.SnaplineMode", ANIMAL_ESP::SnaplineMode);
        SaveFloat(h, "ANIMAL.draw_distance", ANIMAL_ESP::draw_distance);
        SaveFloat(h, "ANIMAL.BoxDist", ANIMAL_ESP::AnimalBoxDist);
        SaveFloat(h, "ANIMAL.NameDist", ANIMAL_ESP::AnimalNameDist);
        SaveFloat(h, "ANIMAL.DistTextDist", ANIMAL_ESP::AnimalDistTextDist);
        SaveFloat(h, "ANIMAL.HPBarDist", ANIMAL_ESP::AnimalHealthBarDist);
        SaveFloat(h, "ANIMAL.SnaplineDist", ANIMAL_ESP::AnimalSnaplineDist);
        SaveBool(h, "ANIMAL.Bear", ANIMAL_ESP::Bear);
        SaveBool(h, "ANIMAL.Wolf", ANIMAL_ESP::Wolf);
        SaveBool(h, "ANIMAL.Boar", ANIMAL_ESP::Boar);
        SaveBool(h, "ANIMAL.Stag", ANIMAL_ESP::Stag);
        SaveBool(h, "ANIMAL.Chicken", ANIMAL_ESP::Chicken);
        SaveBool(h, "ANIMAL.Horse", ANIMAL_ESP::Horse);
        SaveBool(h, "ANIMAL.PolarBear", ANIMAL_ESP::PolarBear);
        SaveBool(h, "ANIMAL.Panther", ANIMAL_ESP::Panther);
        SaveBool(h, "ANIMAL.Tiger", ANIMAL_ESP::Tiger);
        SaveBool(h, "ANIMAL.Snake", ANIMAL_ESP::Snake);
        SaveColor(h, "ANIMAL.color.Box", ANIMAL_ESP::color::Box);
        SaveColor(h, "ANIMAL.color.Name", ANIMAL_ESP::color::Name);
        SaveColor(h, "ANIMAL.color.Distance", ANIMAL_ESP::color::Distance);
        SaveColor(h, "ANIMAL.color.Snaplines", ANIMAL_ESP::color::Snaplines);

        SaveBool(h, "WORLD.Distance", WORLD::Distance);
        SaveBool(h, "WORLD.Hemp", WORLD::Hemp);
        SaveBool(h, "WORLD.Stone", WORLD::Stone);
        SaveBool(h, "WORLD.Metal", WORLD::Metal);
        SaveBool(h, "WORLD.Sulfer", WORLD::Sulfer);
        SaveBool(h, "WORLD.Turret", WORLD::Turret);
        SaveBool(h, "WORLD.Stash", WORLD::Stash);
        SaveBool(h, "WORLD.DroppedItem", WORLD::DroppedItem);
        SaveBool(h, "WORLD.ShotGunTrap", WORLD::ShotGunTrap);
        SaveBool(h, "WORLD.MiniCopter", WORLD::MiniCopter);
        SaveBool(h, "WORLD.Backpack", WORLD::Backpack);
        SaveBool(h, "WORLD.BradlyAPC", WORLD::BradlyAPC);
        SaveBool(h, "WORLD.BodyBag", WORLD::BodyBag);
        SaveFloat(h, "WORLD.draw_distance", WORLD::draw_distance);
        SaveFloat(h, "WORLD.draw_hemp", WORLD::draw_hemp);
        SaveFloat(h, "WORLD.draw_stone", WORLD::draw_stone);
        SaveFloat(h, "WORLD.draw_metal", WORLD::draw_metal);
        SaveFloat(h, "WORLD.draw_sulfur", WORLD::draw_sulfur);
        SaveFloat(h, "WORLD.draw_stash", WORLD::draw_stash);
        SaveFloat(h, "WORLD.draw_corpse", WORLD::draw_corpse);
        SaveFloat(h, "WORLD.draw_turret", WORLD::draw_turret);
        SaveFloat(h, "WORLD.draw_shotguntrap", WORLD::draw_shotguntrap);
        SaveFloat(h, "WORLD.draw_minicopter", WORLD::draw_minicopter);
        SaveFloat(h, "WORLD.draw_backpack", WORLD::draw_backpack);
        SaveFloat(h, "WORLD.draw_bradley", WORLD::draw_bradley);
        SaveFloat(h, "WORLD.draw_dropped", WORLD::draw_dropped);
        SaveFloat(h, "WORLD.draw_samsite", WORLD::draw_samsite);
        SaveFloat(h, "WORLD.draw_beartrap", WORLD::draw_beartrap);
        SaveFloat(h, "WORLD.draw_landmine", WORLD::draw_landmine);
        SaveFloat(h, "WORLD.draw_flameturret", WORLD::draw_flameturret);
        SaveFloat(h, "WORLD.draw_rowboat", WORLD::draw_rowboat);
        SaveFloat(h, "WORLD.draw_rhib", WORLD::draw_rhib);
        SaveFloat(h, "WORLD.draw_kayak", WORLD::draw_kayak);
        SaveFloat(h, "WORLD.draw_submarine", WORLD::draw_submarine);
        SaveFloat(h, "WORLD.draw_tugboat", WORLD::draw_tugboat);
        SaveFloat(h, "WORLD.draw_transportheli", WORLD::draw_transportheli);
        SaveFloat(h, "WORLD.draw_attackheli", WORLD::draw_attackheli);
        SaveFloat(h, "WORLD.draw_balloon", WORLD::draw_balloon);
        SaveFloat(h, "WORLD.draw_motorbike", WORLD::draw_motorbike);
        SaveFloat(h, "WORLD.draw_snowmobile", WORLD::draw_snowmobile);
        SaveFloat(h, "WORLD.draw_barrelbeige", WORLD::draw_barrelbeige);
        SaveFloat(h, "WORLD.draw_barrelblue", WORLD::draw_barrelblue);
        SaveFloat(h, "WORLD.draw_barrelfuel", WORLD::draw_barrelfuel);
        SaveFloat(h, "WORLD.draw_crate_normal", WORLD::draw_crate_normal);
        SaveFloat(h, "WORLD.draw_crate_military", WORLD::draw_crate_military);
        SaveFloat(h, "WORLD.draw_crate_elite", WORLD::draw_crate_elite);
        SaveFloat(h, "WORLD.draw_crate_locked", WORLD::draw_crate_locked);
        SaveFloat(h, "WORLD.draw_lockers", WORLD::draw_lockers);
        SaveFloat(h, "WORLD.draw_bags", WORLD::draw_bags);
        SaveFloat(h, "WORLD.draw_beds", WORLD::draw_beds);
        SaveFloat(h, "WORLD.draw_tc", WORLD::draw_tc);
        SaveFloat(h, "WORLD.draw_vending", WORLD::draw_vending);
        SaveBool(h, "WORLD.StonePickup", WORLD::StonePickup);
        SaveBool(h, "WORLD.MetalPickup", WORLD::MetalPickup);
        SaveBool(h, "WORLD.SulfurPickup", WORLD::SulfurPickup);
        SaveBool(h, "WORLD.WoodPickup", WORLD::WoodPickup);
        SaveBool(h, "WORLD.SamSite", WORLD::SamSite);
        SaveBool(h, "WORLD.BearTrap", WORLD::BearTrap);
        SaveBool(h, "WORLD.Landmine", WORLD::Landmine);
        SaveBool(h, "WORLD.FlameTurret", WORLD::FlameTurret);
        SaveBool(h, "WORLD.Lockers", WORLD::Lockers);
        SaveBool(h, "WORLD.Bags", WORLD::Bags);
        SaveBool(h, "WORLD.Beds", WORLD::Beds);
        SaveBool(h, "WORLD.TC", WORLD::TC);
        SaveBool(h, "WORLD.Vending", WORLD::Vending);
        SaveBool(h, "WORLD.Workbench", WORLD::Workbench);
        SaveBool(h, "WORLD.LargeStorage", WORLD::LargeStorage);
        SaveBool(h, "WORLD.Ladder", WORLD::Ladder);
        SaveBool(h, "WORLD.Generator", WORLD::Generator);
        SaveBool(h, "WORLD.Battery", WORLD::Battery);
        SaveBool(h, "WORLD.BarrelBeige", WORLD::BarrelBeige);
        SaveBool(h, "WORLD.BarrelBlue", WORLD::BarrelBlue);
        SaveBool(h, "WORLD.BarrelFuel", WORLD::BarrelFuel);
        SaveBool(h, "WORLD.CrateNormal", WORLD::CrateNormal);
        SaveBool(h, "WORLD.CrateMilitary", WORLD::CrateMilitary);
        SaveBool(h, "WORLD.CrateElite", WORLD::CrateElite);
        SaveBool(h, "WORLD.CrateLocked", WORLD::CrateLocked);
        SaveBool(h, "WORLD.CrateMedical", WORLD::CrateMedical);
        SaveBool(h, "WORLD.CrateFood", WORLD::CrateFood);
        SaveBool(h, "WORLD.Rowboat", WORLD::Rowboat);
        SaveBool(h, "WORLD.RHIB", WORLD::RHIB);
        SaveBool(h, "WORLD.Kayak", WORLD::Kayak);
        SaveBool(h, "WORLD.Tugboat", WORLD::Tugboat);
        SaveBool(h, "WORLD.Submarine", WORLD::Submarine);
        SaveBool(h, "WORLD.TransportHeli", WORLD::TransportHeli);
        SaveBool(h, "WORLD.AttackHeli", WORLD::AttackHeli);
        SaveBool(h, "WORLD.Balloon", WORLD::Balloon);
        SaveBool(h, "WORLD.Motorbike", WORLD::Motorbike);
        SaveBool(h, "WORLD.MotorbikeSidecar", WORLD::MotorbikeSidecar);
        SaveBool(h, "WORLD.Trike", WORLD::Trike);
        SaveBool(h, "WORLD.Bicycle", WORLD::Bicycle);
        SaveBool(h, "WORLD.Snowmobile", WORLD::Snowmobile);
        SaveBool(h, "WORLD.Shark", WORLD::Shark);
        SaveBool(h, "WORLD.CargoShip", WORLD::CargoShip);
        SaveBool(h, "WORLD.SupplyDrop", WORLD::SupplyDrop);
        SaveColor(h, "WORLD.color.Distance", WORLD::color::Distance_Color);
        SaveColor(h, "WORLD.color.Hemp", WORLD::color::Hemp_Color);
        SaveColor(h, "WORLD.color.BodyBag", WORLD::color::BodyBag_Color);
        SaveColor(h, "WORLD.color.Stone", WORLD::color::Stone_Color);
        SaveColor(h, "WORLD.color.Metal", WORLD::color::Metal_Color);
        SaveColor(h, "WORLD.color.Sulfur", WORLD::color::Sulfer_Color);
        SaveColor(h, "WORLD.color.Turret", WORLD::color::Turret_Color);
        SaveColor(h, "WORLD.color.Stash", WORLD::color::Stash_Color);
        SaveColor(h, "WORLD.color.DroppedItem", WORLD::color::DroppedItem_Color);
        SaveColor(h, "WORLD.color.ShotGunTrap", WORLD::color::ShotGunTrap_Color);
        SaveColor(h, "WORLD.color.MiniCopter", WORLD::color::MiniCopter_Color);
        SaveColor(h, "WORLD.color.Backpack", WORLD::color::Backpack_Color);
        SaveColor(h, "WORLD.color.BradleyAPC", WORLD::color::BradlyAPC);
        SaveColor(h, "WORLD.color.StonePickup", WORLD::color::StonePickup);
        SaveColor(h, "WORLD.color.MetalPickup", WORLD::color::MetalPickup);
        SaveColor(h, "WORLD.color.SulfurPickup", WORLD::color::SulfurPickup);
        SaveColor(h, "WORLD.color.WoodPickup", WORLD::color::WoodPickup);
        SaveColor(h, "WORLD.color.SamSite", WORLD::color::SamSite);
        SaveColor(h, "WORLD.color.BearTrap", WORLD::color::BearTrap);
        SaveColor(h, "WORLD.color.Landmine", WORLD::color::Landmine);
        SaveColor(h, "WORLD.color.FlameTurret", WORLD::color::FlameTurret);
        SaveColor(h, "WORLD.color.Lockers", WORLD::color::Lockers);
        SaveColor(h, "WORLD.color.Bags", WORLD::color::Bags);
        SaveColor(h, "WORLD.color.Beds", WORLD::color::Beds);
        SaveColor(h, "WORLD.color.TC", WORLD::color::TC);
        SaveColor(h, "WORLD.color.Vending", WORLD::color::Vending);
        SaveColor(h, "WORLD.color.Workbench", WORLD::color::Workbench);
        SaveColor(h, "WORLD.color.LargeStorage", WORLD::color::LargeStorage);
        SaveColor(h, "WORLD.color.Ladder", WORLD::color::Ladder);
        SaveColor(h, "WORLD.color.Generator", WORLD::color::Generator);
        SaveColor(h, "WORLD.color.Battery", WORLD::color::Battery);
        SaveColor(h, "WORLD.color.BarrelBeige", WORLD::color::BarrelBeige);
        SaveColor(h, "WORLD.color.BarrelBlue", WORLD::color::BarrelBlue);
        SaveColor(h, "WORLD.color.BarrelFuel", WORLD::color::BarrelFuel);
        SaveColor(h, "WORLD.color.NormalCrate", WORLD::color::NormalCrate);
        SaveColor(h, "WORLD.color.MilitaryCrate", WORLD::color::MilitaryCrate);
        SaveColor(h, "WORLD.color.EliteCrate", WORLD::color::EliteCrate);
        SaveColor(h, "WORLD.color.LockedCrate", WORLD::color::LockedCrate);
        SaveColor(h, "WORLD.color.MedicalCrate", WORLD::color::MedicalCrate);
        SaveColor(h, "WORLD.color.FoodCrate", WORLD::color::FoodCrate);
        SaveColor(h, "WORLD.color.Rowboat", WORLD::color::Rowboat);
        SaveColor(h, "WORLD.color.RHIB", WORLD::color::RHIB);
        SaveColor(h, "WORLD.color.Kayak", WORLD::color::Kayak);
        SaveColor(h, "WORLD.color.Tugboat", WORLD::color::Tugboat);
        SaveColor(h, "WORLD.color.Submarine", WORLD::color::Submarine);
        SaveColor(h, "WORLD.color.TransportHeli", WORLD::color::TransportHeli);
        SaveColor(h, "WORLD.color.AttackHeli", WORLD::color::AttackHeli);
        SaveColor(h, "WORLD.color.Balloon", WORLD::color::Balloon);
        SaveColor(h, "WORLD.color.Motorbike", WORLD::color::Motorbike);
        SaveColor(h, "WORLD.color.Sidecar", WORLD::color::Sidecar);
        SaveColor(h, "WORLD.color.Trike", WORLD::color::Trike);
        SaveColor(h, "WORLD.color.Bicycle", WORLD::color::Bicycle);
        SaveColor(h, "WORLD.color.Snowmobile", WORLD::color::Snowmobile);
        SaveColor(h, "WORLD.color.Shark", WORLD::color::Shark);
        SaveColor(h, "WORLD.color.CargoShip", WORLD::color::CargoShip);
        SaveColor(h, "WORLD.color.SupplyDrop", WORLD::color::SupplyDrop);

        SaveBool(h, "RADAR.Enabled", RADAR::Enabled);
        SaveFloat(h, "RADAR.Size", RADAR::Size);
        SaveFloat(h, "RADAR.Scale", RADAR::Scale);
        SaveFloat(h, "RADAR.DotSize", RADAR::DotSize);
        SaveBool(h, "RADAR.ShowPlayers", RADAR::ShowPlayers);
        SaveBool(h, "RADAR.ShowNPCs", RADAR::ShowNPCs);
        SaveBool(h, "RADAR.ShowAnimals", RADAR::ShowAnimals);
        SaveBool(h, "RADAR.HideSleepers", RADAR::HideSleepers);
        SaveBool(h, "RADAR.RemoveTeam", RADAR::RemoveTeam);
        SaveInt(h, "RADAR.Position", RADAR::Position);
        SaveBool(h, "RADAR.ShowGrid", RADAR::ShowGrid);
        SaveBool(h, "RADAR.Rotate", RADAR::Rotate);
        SaveFloat(h, "RADAR.BackgroundOpacity", RADAR::BackgroundOpacity);
        SaveInt(h, "RADAR.PlayerColorR", (int)(RADAR::PlayerColor.Value.x * 255.f));
        SaveInt(h, "RADAR.PlayerColorG", (int)(RADAR::PlayerColor.Value.y * 255.f));
        SaveInt(h, "RADAR.PlayerColorB", (int)(RADAR::PlayerColor.Value.z * 255.f));
        SaveInt(h, "RADAR.TeammateColorR", (int)(RADAR::TeammateColor.Value.x * 255.f));
        SaveInt(h, "RADAR.TeammateColorG", (int)(RADAR::TeammateColor.Value.y * 255.f));
        SaveInt(h, "RADAR.TeammateColorB", (int)(RADAR::TeammateColor.Value.z * 255.f));
        SaveInt(h, "RADAR.NpcColorR", (int)(RADAR::NpcColor.Value.x * 255.f));
        SaveInt(h, "RADAR.NpcColorG", (int)(RADAR::NpcColor.Value.y * 255.f));
        SaveInt(h, "RADAR.NpcColorB", (int)(RADAR::NpcColor.Value.z * 255.f));
        SaveInt(h, "RADAR.AnimalColorR", (int)(RADAR::AnimalColor.Value.x * 255.f));
        SaveInt(h, "RADAR.AnimalColorG", (int)(RADAR::AnimalColor.Value.y * 255.f));
        SaveInt(h, "RADAR.AnimalColorB", (int)(RADAR::AnimalColor.Value.z * 255.f));
        SaveInt(h, "RADAR.BorderColorR", (int)(RADAR::BorderColor.Value.x * 255.f));
        SaveInt(h, "RADAR.BorderColorG", (int)(RADAR::BorderColor.Value.y * 255.f));
        SaveInt(h, "RADAR.BorderColorB", (int)(RADAR::BorderColor.Value.z * 255.f));

        SaveBool(h, "AIM.Memory", AIMBOT::Memory);
        SaveBool(h, "AIM.Silent", AIMBOT::Silent);
        SaveInt(h, "AIM.FovSize", AIMBOT::FovSize);
        SaveFloat(h, "AIM.SMOOTHING", AIMBOT::SMOOTHING);
        SaveBool(h, "AIM.KeepTarget", AIMBOT::KeepTarget);
        SaveInt(h, "AIM.BoneSelector", AIMBOT::BoneSelector);
        SaveBool(h, "AIM.VisibleOnly", AIMBOT::VisibleOnly);
        SaveBool(h, "AIM.VisibleStrict", AIMBOT::VisibleStrict);
        SaveBool(h, "AIM.IgnoreTeam", AIMBOT::IgnoreTeam);
        SaveBool(h, "AIM.IgnoreWounded", AIMBOT::IgnoreWounded);
        SaveBool(h, "AIM.IgnoreSleepers", AIMBOT::IgnoreSleepers);
        SaveFloat(h, "AIM.MaxDistanceAim", AIMBOT::MaxDistanceAim);
        SaveBool(h, "AIM.DynamicFov", AIMBOT::DynamicFov);
        SaveBool(h, "AIM.WeaponCheck", AIMBOT::WeaponCheck);
        SaveInt(h, "AIM.BonePriority", AIMBOT::BonePriority);
        SaveFloat(h, "AIM.PredictionScale", AIMBOT::PredictionScale);
        SaveBool(h, "AIM.SpinWrites", AIMBOT::SpinWrites);
        SaveBool(h, "AIM.FovCircle", AIMBOT::FovCircle);
        SaveBool(h, "AIM.FovOutline", AIMBOT::FovOutline);
        SaveInt(h, "AIM.FovColorR", (int)(AIMBOT::FovColor.Value.x * 255.f));
        SaveInt(h, "AIM.FovColorG", (int)(AIMBOT::FovColor.Value.y * 255.f));
        SaveInt(h, "AIM.FovColorB", (int)(AIMBOT::FovColor.Value.z * 255.f));
        SaveInt(h, "AIM.FovColorA", (int)(AIMBOT::FovColor.Value.w * 255.f));
        SaveBool(h, "AIM.PredictionIndicator", AIMBOT::PredictionIndicator);
        SaveBool(h, "AIM.TargetLine", AIMBOT::TargetLine);
        SaveBool(h, "AIM.TargetText", AIMBOT::TargetText);
        SaveBool(h, "AIM.TargetLineFromMuzzle", AIMBOT::TargetLineFromMuzzle);
        SaveFloat(h, "AIM.TargetLineThickness", AIMBOT::TargetLineThickness);
        SaveBool(h, "AIM.HumanizeEnabled", AIMBOT::HumanizeEnabled);
        SaveFloat(h, "AIM.JitterAmount", AIMBOT::JitterAmount);
        SaveFloat(h, "AIM.OvershootAmount", AIMBOT::OvershootAmount);
        SaveFloat(h, "AIM.SmoothingVariance", AIMBOT::SmoothingVariance);
        SaveFloat(h, "AIM.MissProbability", AIMBOT::MissProbability);
        SaveInt(h, "AIM.TargetLineColorR", (int)(AIMBOT::color::TargetLine.Value.x * 255.f));
        SaveInt(h, "AIM.TargetLineColorG", (int)(AIMBOT::color::TargetLine.Value.y * 255.f));
        SaveInt(h, "AIM.TargetLineColorB", (int)(AIMBOT::color::TargetLine.Value.z * 255.f));
        SaveInt(h, "AIM.TargetTextColorR", (int)(AIMBOT::color::TargetText.Value.x * 255.f));
        SaveInt(h, "AIM.TargetTextColorG", (int)(AIMBOT::color::TargetText.Value.y * 255.f));
        SaveInt(h, "AIM.TargetTextColorB", (int)(AIMBOT::color::TargetText.Value.z * 255.f));

        SaveBool(h, "MISC.FovChanger", MISC::FovChanger);
        SaveFloat(h, "MISC.FovAmount", MISC::FovAmount);
        SaveBool(h, "MISC.Zoom", MISC::Zoom);
        SaveFloat(h, "MISC.ZoomAmount", MISC::ZoomAmount);
        SaveBool(h, "MISC.RecoilEnabled", MISC::RecoilEnabled);
        SaveFloat(h, "MISC.RecoilModifier", MISC::RecoilModifier);
        SaveBool(h, "MISC.RecoilVariance", MISC::RecoilVariance);
        SaveFloat(h, "MISC.RecoilFloor", MISC::RecoilFloor);
        SaveBool(h, "MISC.AntiAnybrain", MISC::AntiAnybrain);
        SaveBool(h, "MISC.ChangeBurst", MISC::ChangeBurst);
        SaveBool(h, "MISC.QuickBurst", MISC::QuickBurst);
        SaveFloat(h, "MISC.BURSTAMOUNT", MISC::BURSTAMOUNT);
        SaveBool(h, "MISC.Timechanger", MISC::Timechanger);
        SaveInt(h, "MISC.timevalue", MISC::timevalue);
        SaveBool(h, "MISC.BrightNight", MISC::BrightNight);
        SaveBool(h, "MISC.Rayleigh", MISC::Rayleigh);
        SaveFloat(h, "MISC.RayleighVal", MISC::RayleighVal);
        SaveBool(h, "MISC.Mie", MISC::Mie);
        SaveFloat(h, "MISC.MieVal", MISC::MieVal);
        SaveBool(h, "MISC.BrightnessEnv", MISC::BrightnessEnv);
        SaveFloat(h, "MISC.BrightnessEnvVal", MISC::BrightnessEnvVal);
        SaveBool(h, "MISC.StarMod", MISC::StarMod);
        SaveFloat(h, "MISC.StarSize", MISC::StarSize);
        SaveFloat(h, "MISC.StarBrightness", MISC::StarBrightness);
        SaveBool(h, "MISC.MeshMod", MISC::MeshMod);
        SaveFloat(h, "MISC.MeshSize", MISC::MeshSize);
        SaveFloat(h, "MISC.MeshBrightness", MISC::MeshBrightness);
        SaveBool(h, "MISC.RemoveTrees", MISC::RemoveTrees);
        SaveBool(h, "MISC.RemoveGrass", MISC::RemoveGrass);
        SaveBool(h, "MISC.RemoveWater", MISC::RemoveWater);
        SaveBool(h, "MISC.RemoveSky", MISC::RemoveSky);
        SaveBool(h, "MISC.RemoveLayersActive", MISC::RemoveLayersActive);
        SaveBool(h, "MISC.AspectRatio", MISC::AspectRatio);
        SaveFloat(h, "MISC.AspectRatioVal", MISC::AspectRatioVal);
        SaveBool(h, "MISC.TodColors", MISC::TodColors);
        SaveBool(h, "MISC.TodSunMoon", MISC::TodSunMoon);
        SaveBool(h, "MISC.TodLight", MISC::TodLight);
        SaveBool(h, "MISC.TodRay", MISC::TodRay);
        SaveBool(h, "MISC.TodSky", MISC::TodSky);
        SaveBool(h, "MISC.TodCloud", MISC::TodCloud);
        SaveBool(h, "MISC.TodFog", MISC::TodFog);
        SaveBool(h, "MISC.TodAmbient", MISC::TodAmbient);
        SaveFloat(h, "MISC.lightIntensity", MISC::lightIntensity);
        SaveFloat(h, "MISC.ambientMultiplier", MISC::ambientMultiplier);
        SaveFloat(h, "MISC.AmbientSaturation", MISC::AmbientSaturation);
        SaveBool(h, "MISC.ShowServerTime", MISC::ShowServerTime);
        SaveInt(h, "MISC.CrosshairStyle", MISC::CrosshairStyle);
        SaveFloat(h, "MISC.CrosshairSize", MISC::CrosshairSize);
        SaveInt(h, "MISC.CrosshairColorR", (int)(MISC::CrosshairColor.Value.x * 255.f));
        SaveInt(h, "MISC.CrosshairColorG", (int)(MISC::CrosshairColor.Value.y * 255.f));
        SaveInt(h, "MISC.CrosshairColorB", (int)(MISC::CrosshairColor.Value.z * 255.f));
        SaveBool(h, "MISC.DebugCamera", MISC::DebugCamera);
        SaveBool(h, "MISC.AdminFlags", MISC::AdminFlags);
        SaveBool(h, "MISC.CombatMode", MISC::CombatMode);
        SaveBool(h, "MISC.DrawFlags", MISC::DrawFlags);
        SaveBool(h, "MISC.ShowEspRangeOverlay", MISC::ShowEspRangeOverlay);
        SaveBool(h, "MISC.SilentShot", MISC::SilentShot);
        SaveBool(h, "MISC.usetouchUnderWater", MISC::usetouchUnderWater);
        SaveBool(h, "MISC.MiniGunMagicBullet", MISC::MiniGunMagicBullet);
        SaveBool(h, "MISC.NoMeleeCoolDown", MISC::NoMeleeCoolDown);
        SaveBool(h, "MISC.NoSpread", MISC::NoSpread);
        SaveBool(h, "MISC.Automatic", MISC::Automatic);
        SaveBool(h, "MISC.Aimsway", MISC::Aimsway);
        SaveBool(h, "MISC.NoPunch", MISC::NoPunch);
        SaveBool(h, "MISC.RapidFire", MISC::RapidFire);
        SaveBool(h, "MISC.SuperMelee", MISC::SuperMelee);
        SaveBool(h, "MISC.InstantBow", MISC::InstantBow);
        SaveBool(h, "MISC.HitSound", MISC::HitSound);
        SaveBool(h, "MISC.InstantEoka", MISC::InstantEoka);
        SaveBool(h, "MISC.FastBow", MISC::FastBow);
        SaveBool(h, "MISC.NoPullback", MISC::NoPullback);
        SaveBool(h, "MISC.InstantCompound", MISC::InstantCompound);
        SaveBool(h, "MISC.Longhand", MISC::Longhand);
        SaveBool(h, "MISC.NoHeavySlow", MISC::NoHeavySlow);
        SaveBool(h, "MISC.NoAnim", MISC::NoAnim);
        SaveBool(h, "MISC.NoBob", MISC::NoBob);
        SaveBool(h, "MISC.NoSway", MISC::NoSway);
        SaveBool(h, "MISC.NoLower", MISC::NoLower);
        SaveBool(h, "MISC.AlwaysSprint", MISC::AlwaysSprint);
        SaveBool(h, "MISC.omniSprint", MISC::omniSprint);
        SaveBool(h, "MISC.JumpShot", MISC::JumpShot);
        SaveBool(h, "MISC.WalkonWalls", MISC::WalkonWalls);
        SaveBool(h, "MISC.NoFall", MISC::NoFall);
        SaveBool(h, "MISC.ShootInAir", MISC::ShootInAir);
        SaveBool(h, "MISC.ReducedGravity", MISC::ReducedGravity);
        SaveFloat(h, "MISC.GravityValue", MISC::GravityValue);
        SaveBool(h, "MISC.MoonJump", MISC::MoonJump);
        SaveBool(h, "MISC.Spinbot", MISC::Spinbot);
        SaveBool(h, "MISC.mincoptershoot", MISC::mincoptershoot);
        SaveBool(h, "MISC.AimWhileHeavy", MISC::AimWhileHeavy);
        SaveBool(h, "MISC.FASTHORSE", MISC::FASTHORSE);
        SaveBool(h, "MISC.Fly", MISC::Fly);
        SaveFloat(h, "MISC.FlySpeed", MISC::FlySpeed);
        SaveBool(h, "MISC.FlySafeMode", MISC::FlySafeMode);
        SaveBool(h, "MISC.HighJump", MISC::HighJump);
        SaveFloat(h, "MISC.HighJumpAngle", MISC::HighJumpAngle);
        SaveInt(h, "MISC.FlyKey", MISC::FlyKey);
        SaveBool(h, "MISC.SpeedHack", MISC::SpeedHack);
        SaveInt(h, "MISC.SpeedKey", MISC::SpeedKey);
        SaveFloat(h, "MISC.SpeedValue", MISC::SpeedValue);
        SaveBool(h, "MISC.Spiderman", MISC::Spiderman);
        SaveBool(h, "MISC.WalkOnWater", MISC::WalkOnWater);
        SaveBool(h, "MISC.InfJump", MISC::InfJump);
        SaveBool(h, "MISC.NoWearOverlay", MISC::NoWearOverlay);
        SaveFloat(h, "MISC.SpreadScale", MISC::SpreadScale);
        SaveFloat(h, "MISC.FireRateScale", MISC::FireRateScale);
        SaveBool(h, "SETTINGS.AdminFlag", SETTINGS::AdminFlag);
        SaveBool(h, "SETTINGS.BattleMode", SETTINGS::BattleMode);
        SaveFloat(h, "SETTINGS.ROTSPEED", SETTINGS::ROTSPEED);
        SaveInt(h, "SETTINGS.Language", SETTINGS::Language);
        SaveBool(h, "BATTLE.Players", BATTLE::Players);
        SaveBool(h, "BATTLE.Aimbot", BATTLE::Aimbot);
        SaveBool(h, "BATTLE.World", BATTLE::World);
        SaveBool(h, "BATTLE.NPC", BATTLE::NPC);
        SaveBool(h, "BATTLE.Animals", BATTLE::Animals);
        SaveBool(h, "BATTLE.Radar", BATTLE::Radar);

        for (const auto& [id, bind] : g_Hotkeys) {
            if (bind.key == 0) continue;
            if (id == "COMBAT.Recoil") continue;
            char line[128];
            wsprintfA(line, "HK.%s=%d:%d\r\n", id.c_str(), bind.key, bind.mode);
            DWORD written;
            WriteFile(h, line, lstrlenA(line), &written, NULL);
        }

        CloseHandle(h);
        SetStatus("Config saved");
        return true;
    }

    inline void ParseLine(char* line) {
        char* eq = nullptr;
        for (char* p = line; *p; p++) { if (*p == '=') { eq = p; break; } }
        if (!eq) return;
        *eq = 0;
        char* key = line;
        char* val = eq + 1;
        while (*val == ' ') val++;
        char* end = val + lstrlenA(val) - 1;
        while (end > val && (*end == '\r' || *end == '\n' || *end == ' ')) { *end = 0; end--; }

        if (lstrlenA(key) > 3 && key[0] == 'H' && key[1] == 'K' && key[2] == '.') {
            char* colon = val;
            while (*colon && *colon != ':') colon++;
            if (*colon == ':') {
                *colon = 0;
                const char* hkId = key + 3;
                if (lstrcmpA(hkId, "COMBAT.Recoil") == 0) return;
                g_Hotkeys[hkId].key = atoi(val);
                int mode = atoi(colon + 1);
                if (mode < HK_TOGGLE || mode > HK_DISABLED) mode = HK_HOLD;
                g_Hotkeys[hkId].mode = mode;
            }
            return;
        }

        int iv = 0; float fv = 0.f;
        bool hasDot = false;
        for (char* c = val; *c; c++) if (*c == '.') hasDot = true;

        if (hasDot) {
            int w = 0, f = 0, neg = 0;
            char* p = val;
            if (*p == '-') { neg = 1; p++; }
            while (*p >= '0' && *p <= '9') { w = w * 10 + (*p - '0'); p++; }
            if (*p == '.') { p++; int d = 1; while (*p >= '0' && *p <= '9') { f = f * 10 + (*p - '0'); d *= 10; p++; } fv = (float)w + (float)f / (float)d; }
            else fv = (float)w;
            if (neg) fv = -fv;
        }
        iv = 0;
        { int neg = 0; char* p = val; if (*p == '-') { neg = 1; p++; } while (*p >= '0' && *p <= '9') { iv = iv * 10 + (*p - '0'); p++; } if (neg) iv = -iv; }

        #define LOAD_BOOL(k, v) if (lstrcmpA(key, k) == 0) { v = (iv != 0); return; }
        #define LOAD_INT(k, v)  if (lstrcmpA(key, k) == 0) { v = iv; return; }
        #define LOAD_FLOAT(k, v) if (lstrcmpA(key, k) == 0) { v = fv; return; }
        #define LOAD_COLOR(k, v) if (lstrcmpA(key, k) == 0) { \
            int _r=255,_g=255,_b=255,_a=255; \
            sscanf_s(val, "%d,%d,%d,%d", &_r, &_g, &_b, &_a); \
            v = ImColor((float)_r/255.f, (float)_g/255.f, (float)_b/255.f, (float)_a/255.f); \
            return; \
        }

        LOAD_BOOL("ESP.Box", ESP::Box);
        LOAD_BOOL("ESP.Enabled", ESP::ESPEnabled);
        LOAD_BOOL("ESP.CornerBox", ESP::CornerBox);
        LOAD_BOOL("ESP.FilledBox", ESP::FilledBox);
        LOAD_BOOL("ESP.Name", ESP::Name);
        LOAD_BOOL("ESP.Distance", ESP::Distance);
        LOAD_BOOL("ESP.Skeleton", ESP::Skeleton);
        LOAD_BOOL("ESP.HeadCircle", ESP::HeadCircle);
        LOAD_BOOL("ESP.HealthBar", ESP::HealthBar);
        LOAD_BOOL("ESP.Weapon", ESP::Weapon);
        LOAD_BOOL("ESP.SnapLines", ESP::SnapLines);
        LOAD_BOOL("ESP.OFFArrows", ESP::OFFArrows);
        LOAD_BOOL("ESP.VisCheck", ESP::VisCheck);
        LOAD_INT("ESP.VisMode", ESP::VisMode);
        LOAD_INT("ESP.VisSamples", ESP::VisSamples);
        LOAD_INT("ESP.VisHoldMs", ESP::VisHoldMs);
        LOAD_FLOAT("ESP.OccluderDist", ESP::OccluderDist);
        LOAD_BOOL("ESP.TeamID", ESP::TeamID);
        LOAD_BOOL("ESP.hotbar_text", ESP::hotbar_text);
        LOAD_BOOL("ESP.HotbarIcons", ESP::HotbarIcons);
        LOAD_BOOL("ESP.RemoveWounded", ESP::RemoveWounded);
        LOAD_BOOL("ESP.HighlightWounded", ESP::HighlightWounded);
        LOAD_BOOL("ESP.RemoveSleepers", ESP::RemoveSleepers);
        LOAD_BOOL("ESP.HighlightSleepers", ESP::HighlightSleepers);
        LOAD_BOOL("ESP.RemoveTeam", ESP::RemoveTeam);
        LOAD_BOOL("ESP.DrawTextBackground", ESP::DrawTextBackground);
        LOAD_BOOL("ESP.SkeletonOutline", ESP::SkeletonOutline);
        LOAD_INT("ESP.SkeletonThickness", ESP::SkeletonThickness);
        LOAD_BOOL("ESP.BoxOutline", ESP::BoxOutline);
        LOAD_INT("ESP.BoxThickness", ESP::BoxThickness);
        LOAD_BOOL("ESP.SnaplineOutline", ESP::SnaplineOutline);
        LOAD_INT("ESP.SnaplineThickness", ESP::SnaplineThickness);
        LOAD_BOOL("ESP.Clothing", ESP::Clothing);
        LOAD_BOOL("ESP.ItemList", ESP::ItemList);
        LOAD_BOOL("ESP.AmmoBar", ESP::AmmoBar);
        LOAD_BOOL("ESP.PlayerInventoryPanel", ESP::PlayerInventoryPanel);
        LOAD_BOOL("ESP.MovementTrails", ESP::MovementTrails);
        LOAD_BOOL("ESP.BulletTracers", ESP::BulletTracers);
        LOAD_BOOL("ESP.ShowVisibility", ESP::ShowVisibility);
        LOAD_FLOAT("ESP.TracerThickness", ESP::TracerThickness);
        LOAD_BOOL("ESP.TracerOutline", ESP::TracerOutline);
        LOAD_FLOAT("ESP.EspFontSize", ESP::EspFontSize);
        LOAD_INT("ESP.DistanceBracketStyle", ESP::DistanceBracketStyle);
        LOAD_BOOL("ESP.ReloadBar", ESP::ReloadBar);
        LOAD_FLOAT("ESP.draw_distance", ESP::draw_distance);
        LOAD_BOOL("ESP.ESPAdvanced", ESP::ESPAdvanced);
        LOAD_FLOAT("ESP.BoxDist", ESP::BoxDist);
        LOAD_FLOAT("ESP.SkeletonDist", ESP::SkeletonDist);
        LOAD_FLOAT("ESP.NameDist", ESP::NameDist);
        LOAD_FLOAT("ESP.WeaponDist", ESP::WeaponDist);
        LOAD_FLOAT("ESP.HealthBarDist", ESP::HealthBarDist);
        LOAD_FLOAT("ESP.SnaplineDist", ESP::SnaplineDist);
        LOAD_FLOAT("ESP.HeadCircleDist", ESP::HeadCircleDist);
        LOAD_FLOAT("ESP.OFFArrowDist", ESP::OFFArrowDist);
        LOAD_INT("ESP.SnaplineMode", ESP::SnaplineMode);
        LOAD_BOOL("ESP.VisGhost", ESP::VisGhost);
        LOAD_INT("ESP.VisGhostMs", ESP::VisGhostMs);
        LOAD_COLOR("ESP.color.Tracer", ESP::TracerColor);
        LOAD_COLOR("ESP.color.Box", ESP::color::Box);
        LOAD_COLOR("ESP.color.FilledBox", ESP::color::FilledBox);
        LOAD_COLOR("ESP.color.Distance", ESP::color::Distance);
        LOAD_COLOR("ESP.color.Snaplines", ESP::color::Snaplines);
        LOAD_COLOR("ESP.color.HeadCircle", ESP::color::HeadCircle);
        LOAD_COLOR("ESP.color.Weapon", ESP::color::Weapon);
        LOAD_COLOR("ESP.color.Name", ESP::color::Name);
        LOAD_COLOR("ESP.color.Skeleton", ESP::color::Skeleton);
        LOAD_COLOR("ESP.color.Wounded", ESP::color::Wounded);
        LOAD_COLOR("ESP.color.Sleepers", ESP::color::Sleepers);
        LOAD_COLOR("ESP.color.Team", ESP::color::Team);
        LOAD_COLOR("ESP.color.OFFArrow", ESP::color::OFFArrowColor);
        LOAD_COLOR("ESP.color.Visible", ESP::color::Visible);
        LOAD_COLOR("ESP.color.Invisible", ESP::color::Invisible);
        LOAD_COLOR("ESP.color.SkeletonVisible", ESP::color::SkeletonVisible);
        LOAD_COLOR("ESP.color.SkeletonInvisible", ESP::color::SkeletonInvisible);
        LOAD_COLOR("ESP.color.HealthBar", ESP::color::HealthBar);
        LOAD_COLOR("ESP.color.Ghost", ESP::color::Ghost);
        LOAD_COLOR("ESP.color.Teammate", ESP::color::Teammate);

        LOAD_BOOL("NPC.Enabled", NPC_ESP::Enabled);
        LOAD_BOOL("NPC.ESPAdvanced", NPC_ESP::ESPAdvanced);
        LOAD_BOOL("NPC.Box", NPC_ESP::Box);
        LOAD_BOOL("NPC.Name", NPC_ESP::Name);
        LOAD_BOOL("NPC.Distance", NPC_ESP::Distance);
        LOAD_BOOL("NPC.HealthBar", NPC_ESP::HealthBar);
        LOAD_BOOL("NPC.Skeleton", NPC_ESP::Skeleton);
        LOAD_BOOL("NPC.HeldItem", NPC_ESP::HeldItem);
        LOAD_BOOL("NPC.SnapLines", NPC_ESP::SnapLines);
        LOAD_INT("NPC.SnaplineMode", NPC_ESP::SnaplineMode);
        LOAD_FLOAT("NPC.draw_distance", NPC_ESP::draw_distance);
        LOAD_FLOAT("NPC.BoxDist", NPC_ESP::NPCBoxDist);
        LOAD_FLOAT("NPC.NameDist", NPC_ESP::NPCNameDist);
        LOAD_FLOAT("NPC.DistTextDist", NPC_ESP::NPCDistTextDist);
        LOAD_FLOAT("NPC.HPBarDist", NPC_ESP::NPCHealthBarDist);
        LOAD_FLOAT("NPC.SkeletonDist", NPC_ESP::NPCSkeletonDist);
        LOAD_FLOAT("NPC.HeldItemDist", NPC_ESP::NPCHeldItemDist);
        LOAD_FLOAT("NPC.SnaplineDist", NPC_ESP::NPCSnaplineDist);
        LOAD_BOOL("NPC.Scientists", NPC_ESP::Scientists);
        LOAD_BOOL("NPC.TunnelDweller", NPC_ESP::TunnelDweller);
        LOAD_BOOL("NPC.UnderwaterDweller", NPC_ESP::UnderwaterDweller);
        LOAD_COLOR("NPC.color.Box", NPC_ESP::color::Box);
        LOAD_COLOR("NPC.color.Name", NPC_ESP::color::Name);
        LOAD_COLOR("NPC.color.Distance", NPC_ESP::color::Distance);
        LOAD_COLOR("NPC.color.Skeleton", NPC_ESP::color::Skeleton);
        LOAD_COLOR("NPC.color.Snaplines", NPC_ESP::color::Snaplines);

        LOAD_BOOL("ANIMAL.Enabled", ANIMAL_ESP::Enabled);
        LOAD_BOOL("ANIMAL.ESPAdvanced", ANIMAL_ESP::ESPAdvanced);
        LOAD_BOOL("ANIMAL.Box", ANIMAL_ESP::Box);
        LOAD_BOOL("ANIMAL.Name", ANIMAL_ESP::Name);
        LOAD_BOOL("ANIMAL.Distance", ANIMAL_ESP::Distance);
        LOAD_BOOL("ANIMAL.HealthBar", ANIMAL_ESP::HealthBar);
        LOAD_BOOL("ANIMAL.SnapLines", ANIMAL_ESP::SnapLines);
        LOAD_INT("ANIMAL.SnaplineMode", ANIMAL_ESP::SnaplineMode);
        LOAD_FLOAT("ANIMAL.draw_distance", ANIMAL_ESP::draw_distance);
        LOAD_FLOAT("ANIMAL.BoxDist", ANIMAL_ESP::AnimalBoxDist);
        LOAD_FLOAT("ANIMAL.NameDist", ANIMAL_ESP::AnimalNameDist);
        LOAD_FLOAT("ANIMAL.DistTextDist", ANIMAL_ESP::AnimalDistTextDist);
        LOAD_FLOAT("ANIMAL.HPBarDist", ANIMAL_ESP::AnimalHealthBarDist);
        LOAD_FLOAT("ANIMAL.SnaplineDist", ANIMAL_ESP::AnimalSnaplineDist);
        LOAD_BOOL("ANIMAL.Bear", ANIMAL_ESP::Bear);
        LOAD_BOOL("ANIMAL.Wolf", ANIMAL_ESP::Wolf);
        LOAD_BOOL("ANIMAL.Boar", ANIMAL_ESP::Boar);
        LOAD_BOOL("ANIMAL.Stag", ANIMAL_ESP::Stag);
        LOAD_BOOL("ANIMAL.Chicken", ANIMAL_ESP::Chicken);
        LOAD_BOOL("ANIMAL.Horse", ANIMAL_ESP::Horse);
        LOAD_BOOL("ANIMAL.PolarBear", ANIMAL_ESP::PolarBear);
        LOAD_BOOL("ANIMAL.Panther", ANIMAL_ESP::Panther);
        LOAD_BOOL("ANIMAL.Tiger", ANIMAL_ESP::Tiger);
        LOAD_BOOL("ANIMAL.Snake", ANIMAL_ESP::Snake);
        LOAD_COLOR("ANIMAL.color.Box", ANIMAL_ESP::color::Box);
        LOAD_COLOR("ANIMAL.color.Name", ANIMAL_ESP::color::Name);
        LOAD_COLOR("ANIMAL.color.Distance", ANIMAL_ESP::color::Distance);
        LOAD_COLOR("ANIMAL.color.Snaplines", ANIMAL_ESP::color::Snaplines);

        LOAD_BOOL("WORLD.Distance", WORLD::Distance);
        LOAD_BOOL("WORLD.Hemp", WORLD::Hemp);
        LOAD_BOOL("WORLD.Stone", WORLD::Stone);
        LOAD_BOOL("WORLD.Metal", WORLD::Metal);
        LOAD_BOOL("WORLD.Sulfer", WORLD::Sulfer);
        LOAD_BOOL("WORLD.Turret", WORLD::Turret);
        LOAD_BOOL("WORLD.Stash", WORLD::Stash);
        LOAD_BOOL("WORLD.DroppedItem", WORLD::DroppedItem);
        LOAD_BOOL("WORLD.ShotGunTrap", WORLD::ShotGunTrap);
        LOAD_BOOL("WORLD.MiniCopter", WORLD::MiniCopter);
        LOAD_BOOL("WORLD.Backpack", WORLD::Backpack);
        LOAD_BOOL("WORLD.BradlyAPC", WORLD::BradlyAPC);
        LOAD_BOOL("WORLD.BodyBag", WORLD::BodyBag);
        LOAD_FLOAT("WORLD.draw_distance", WORLD::draw_distance);
        LOAD_FLOAT("WORLD.draw_hemp", WORLD::draw_hemp);
        LOAD_FLOAT("WORLD.draw_stone", WORLD::draw_stone);
        LOAD_FLOAT("WORLD.draw_metal", WORLD::draw_metal);
        LOAD_FLOAT("WORLD.draw_sulfur", WORLD::draw_sulfur);
        LOAD_FLOAT("WORLD.draw_stash", WORLD::draw_stash);
        LOAD_FLOAT("WORLD.draw_corpse", WORLD::draw_corpse);
        LOAD_FLOAT("WORLD.draw_turret", WORLD::draw_turret);
        LOAD_FLOAT("WORLD.draw_shotguntrap", WORLD::draw_shotguntrap);
        LOAD_FLOAT("WORLD.draw_minicopter", WORLD::draw_minicopter);
        LOAD_FLOAT("WORLD.draw_backpack", WORLD::draw_backpack);
        LOAD_FLOAT("WORLD.draw_bradley", WORLD::draw_bradley);
        LOAD_FLOAT("WORLD.draw_dropped", WORLD::draw_dropped);
        LOAD_FLOAT("WORLD.draw_samsite", WORLD::draw_samsite);
        LOAD_FLOAT("WORLD.draw_beartrap", WORLD::draw_beartrap);
        LOAD_FLOAT("WORLD.draw_landmine", WORLD::draw_landmine);
        LOAD_FLOAT("WORLD.draw_flameturret", WORLD::draw_flameturret);
        LOAD_FLOAT("WORLD.draw_rowboat", WORLD::draw_rowboat);
        LOAD_FLOAT("WORLD.draw_rhib", WORLD::draw_rhib);
        LOAD_FLOAT("WORLD.draw_kayak", WORLD::draw_kayak);
        LOAD_FLOAT("WORLD.draw_submarine", WORLD::draw_submarine);
        LOAD_FLOAT("WORLD.draw_tugboat", WORLD::draw_tugboat);
        LOAD_FLOAT("WORLD.draw_transportheli", WORLD::draw_transportheli);
        LOAD_FLOAT("WORLD.draw_attackheli", WORLD::draw_attackheli);
        LOAD_FLOAT("WORLD.draw_balloon", WORLD::draw_balloon);
        LOAD_FLOAT("WORLD.draw_motorbike", WORLD::draw_motorbike);
        LOAD_FLOAT("WORLD.draw_snowmobile", WORLD::draw_snowmobile);
        LOAD_FLOAT("WORLD.draw_barrelbeige", WORLD::draw_barrelbeige);
        LOAD_FLOAT("WORLD.draw_barrelblue", WORLD::draw_barrelblue);
        LOAD_FLOAT("WORLD.draw_barrelfuel", WORLD::draw_barrelfuel);
        LOAD_FLOAT("WORLD.draw_crate_normal", WORLD::draw_crate_normal);
        LOAD_FLOAT("WORLD.draw_crate_military", WORLD::draw_crate_military);
        LOAD_FLOAT("WORLD.draw_crate_elite", WORLD::draw_crate_elite);
        LOAD_FLOAT("WORLD.draw_crate_locked", WORLD::draw_crate_locked);
        LOAD_FLOAT("WORLD.draw_lockers", WORLD::draw_lockers);
        LOAD_FLOAT("WORLD.draw_bags", WORLD::draw_bags);
        LOAD_FLOAT("WORLD.draw_beds", WORLD::draw_beds);
        LOAD_FLOAT("WORLD.draw_tc", WORLD::draw_tc);
        LOAD_FLOAT("WORLD.draw_vending", WORLD::draw_vending);
        LOAD_BOOL("WORLD.StonePickup", WORLD::StonePickup);
        LOAD_BOOL("WORLD.MetalPickup", WORLD::MetalPickup);
        LOAD_BOOL("WORLD.SulfurPickup", WORLD::SulfurPickup);
        LOAD_BOOL("WORLD.WoodPickup", WORLD::WoodPickup);
        LOAD_BOOL("WORLD.SamSite", WORLD::SamSite);
        LOAD_BOOL("WORLD.BearTrap", WORLD::BearTrap);
        LOAD_BOOL("WORLD.Landmine", WORLD::Landmine);
        LOAD_BOOL("WORLD.FlameTurret", WORLD::FlameTurret);
        LOAD_BOOL("WORLD.Lockers", WORLD::Lockers);
        LOAD_BOOL("WORLD.Bags", WORLD::Bags);
        LOAD_BOOL("WORLD.Beds", WORLD::Beds);
        LOAD_BOOL("WORLD.TC", WORLD::TC);
        LOAD_BOOL("WORLD.Vending", WORLD::Vending);
        LOAD_BOOL("WORLD.Workbench", WORLD::Workbench);
        LOAD_BOOL("WORLD.LargeStorage", WORLD::LargeStorage);
        LOAD_BOOL("WORLD.Ladder", WORLD::Ladder);
        LOAD_BOOL("WORLD.Generator", WORLD::Generator);
        LOAD_BOOL("WORLD.Battery", WORLD::Battery);
        LOAD_BOOL("WORLD.BarrelBeige", WORLD::BarrelBeige);
        LOAD_BOOL("WORLD.BarrelBlue", WORLD::BarrelBlue);
        LOAD_BOOL("WORLD.BarrelFuel", WORLD::BarrelFuel);
        LOAD_BOOL("WORLD.CrateNormal", WORLD::CrateNormal);
        LOAD_BOOL("WORLD.CrateMilitary", WORLD::CrateMilitary);
        LOAD_BOOL("WORLD.CrateElite", WORLD::CrateElite);
        LOAD_BOOL("WORLD.CrateLocked", WORLD::CrateLocked);
        LOAD_BOOL("WORLD.CrateMedical", WORLD::CrateMedical);
        LOAD_BOOL("WORLD.CrateFood", WORLD::CrateFood);
        LOAD_BOOL("WORLD.Rowboat", WORLD::Rowboat);
        LOAD_BOOL("WORLD.RHIB", WORLD::RHIB);
        LOAD_BOOL("WORLD.Kayak", WORLD::Kayak);
        LOAD_BOOL("WORLD.Tugboat", WORLD::Tugboat);
        LOAD_BOOL("WORLD.Submarine", WORLD::Submarine);
        LOAD_BOOL("WORLD.TransportHeli", WORLD::TransportHeli);
        LOAD_BOOL("WORLD.AttackHeli", WORLD::AttackHeli);
        LOAD_BOOL("WORLD.Balloon", WORLD::Balloon);
        LOAD_BOOL("WORLD.Motorbike", WORLD::Motorbike);
        LOAD_BOOL("WORLD.MotorbikeSidecar", WORLD::MotorbikeSidecar);
        LOAD_BOOL("WORLD.Trike", WORLD::Trike);
        LOAD_BOOL("WORLD.Bicycle", WORLD::Bicycle);
        LOAD_BOOL("WORLD.Snowmobile", WORLD::Snowmobile);
        LOAD_BOOL("WORLD.Shark", WORLD::Shark);
        LOAD_BOOL("WORLD.CargoShip", WORLD::CargoShip);
        LOAD_BOOL("WORLD.SupplyDrop", WORLD::SupplyDrop);
        LOAD_COLOR("WORLD.color.Distance", WORLD::color::Distance_Color);
        LOAD_COLOR("WORLD.color.Hemp", WORLD::color::Hemp_Color);
        LOAD_COLOR("WORLD.color.BodyBag", WORLD::color::BodyBag_Color);
        LOAD_COLOR("WORLD.color.Stone", WORLD::color::Stone_Color);
        LOAD_COLOR("WORLD.color.Metal", WORLD::color::Metal_Color);
        LOAD_COLOR("WORLD.color.Sulfur", WORLD::color::Sulfer_Color);
        LOAD_COLOR("WORLD.color.Turret", WORLD::color::Turret_Color);
        LOAD_COLOR("WORLD.color.Stash", WORLD::color::Stash_Color);
        LOAD_COLOR("WORLD.color.DroppedItem", WORLD::color::DroppedItem_Color);
        LOAD_COLOR("WORLD.color.ShotGunTrap", WORLD::color::ShotGunTrap_Color);
        LOAD_COLOR("WORLD.color.MiniCopter", WORLD::color::MiniCopter_Color);
        LOAD_COLOR("WORLD.color.Backpack", WORLD::color::Backpack_Color);
        LOAD_COLOR("WORLD.color.BradleyAPC", WORLD::color::BradlyAPC);
        LOAD_COLOR("WORLD.color.StonePickup", WORLD::color::StonePickup);
        LOAD_COLOR("WORLD.color.MetalPickup", WORLD::color::MetalPickup);
        LOAD_COLOR("WORLD.color.SulfurPickup", WORLD::color::SulfurPickup);
        LOAD_COLOR("WORLD.color.WoodPickup", WORLD::color::WoodPickup);
        LOAD_COLOR("WORLD.color.SamSite", WORLD::color::SamSite);
        LOAD_COLOR("WORLD.color.BearTrap", WORLD::color::BearTrap);
        LOAD_COLOR("WORLD.color.Landmine", WORLD::color::Landmine);
        LOAD_COLOR("WORLD.color.FlameTurret", WORLD::color::FlameTurret);
        LOAD_COLOR("WORLD.color.Lockers", WORLD::color::Lockers);
        LOAD_COLOR("WORLD.color.Bags", WORLD::color::Bags);
        LOAD_COLOR("WORLD.color.Beds", WORLD::color::Beds);
        LOAD_COLOR("WORLD.color.TC", WORLD::color::TC);
        LOAD_COLOR("WORLD.color.Vending", WORLD::color::Vending);
        LOAD_COLOR("WORLD.color.Workbench", WORLD::color::Workbench);
        LOAD_COLOR("WORLD.color.LargeStorage", WORLD::color::LargeStorage);
        LOAD_COLOR("WORLD.color.Ladder", WORLD::color::Ladder);
        LOAD_COLOR("WORLD.color.Generator", WORLD::color::Generator);
        LOAD_COLOR("WORLD.color.Battery", WORLD::color::Battery);
        LOAD_COLOR("WORLD.color.BarrelBeige", WORLD::color::BarrelBeige);
        LOAD_COLOR("WORLD.color.BarrelBlue", WORLD::color::BarrelBlue);
        LOAD_COLOR("WORLD.color.BarrelFuel", WORLD::color::BarrelFuel);
        LOAD_COLOR("WORLD.color.NormalCrate", WORLD::color::NormalCrate);
        LOAD_COLOR("WORLD.color.MilitaryCrate", WORLD::color::MilitaryCrate);
        LOAD_COLOR("WORLD.color.EliteCrate", WORLD::color::EliteCrate);
        LOAD_COLOR("WORLD.color.LockedCrate", WORLD::color::LockedCrate);
        LOAD_COLOR("WORLD.color.MedicalCrate", WORLD::color::MedicalCrate);
        LOAD_COLOR("WORLD.color.FoodCrate", WORLD::color::FoodCrate);
        LOAD_COLOR("WORLD.color.Rowboat", WORLD::color::Rowboat);
        LOAD_COLOR("WORLD.color.RHIB", WORLD::color::RHIB);
        LOAD_COLOR("WORLD.color.Kayak", WORLD::color::Kayak);
        LOAD_COLOR("WORLD.color.Tugboat", WORLD::color::Tugboat);
        LOAD_COLOR("WORLD.color.Submarine", WORLD::color::Submarine);
        LOAD_COLOR("WORLD.color.TransportHeli", WORLD::color::TransportHeli);
        LOAD_COLOR("WORLD.color.AttackHeli", WORLD::color::AttackHeli);
        LOAD_COLOR("WORLD.color.Balloon", WORLD::color::Balloon);
        LOAD_COLOR("WORLD.color.Motorbike", WORLD::color::Motorbike);
        LOAD_COLOR("WORLD.color.Sidecar", WORLD::color::Sidecar);
        LOAD_COLOR("WORLD.color.Trike", WORLD::color::Trike);
        LOAD_COLOR("WORLD.color.Bicycle", WORLD::color::Bicycle);
        LOAD_COLOR("WORLD.color.Snowmobile", WORLD::color::Snowmobile);
        LOAD_COLOR("WORLD.color.Shark", WORLD::color::Shark);
        LOAD_COLOR("WORLD.color.CargoShip", WORLD::color::CargoShip);
        LOAD_COLOR("WORLD.color.SupplyDrop", WORLD::color::SupplyDrop);

        LOAD_BOOL("RADAR.Enabled", RADAR::Enabled);
        LOAD_FLOAT("RADAR.Size", RADAR::Size);
        LOAD_FLOAT("RADAR.Scale", RADAR::Scale);
        LOAD_FLOAT("RADAR.DotSize", RADAR::DotSize);
        LOAD_BOOL("RADAR.ShowPlayers", RADAR::ShowPlayers);
        LOAD_BOOL("RADAR.ShowNPCs", RADAR::ShowNPCs);
        LOAD_BOOL("RADAR.ShowAnimals", RADAR::ShowAnimals);
        LOAD_BOOL("RADAR.HideSleepers", RADAR::HideSleepers);
        LOAD_BOOL("RADAR.RemoveTeam", RADAR::RemoveTeam);
        LOAD_INT("RADAR.Position", RADAR::Position);
        LOAD_BOOL("RADAR.ShowGrid", RADAR::ShowGrid);
        LOAD_BOOL("RADAR.Rotate", RADAR::Rotate);
        LOAD_FLOAT("RADAR.BackgroundOpacity", RADAR::BackgroundOpacity);
        if (lstrcmpA(key, "RADAR.PlayerColorR") == 0) { RADAR::PlayerColor.Value.x = iv / 255.f; return; }
        if (lstrcmpA(key, "RADAR.PlayerColorG") == 0) { RADAR::PlayerColor.Value.y = iv / 255.f; return; }
        if (lstrcmpA(key, "RADAR.PlayerColorB") == 0) { RADAR::PlayerColor.Value.z = iv / 255.f; return; }
        if (lstrcmpA(key, "RADAR.TeammateColorR") == 0) { RADAR::TeammateColor.Value.x = iv / 255.f; return; }
        if (lstrcmpA(key, "RADAR.TeammateColorG") == 0) { RADAR::TeammateColor.Value.y = iv / 255.f; return; }
        if (lstrcmpA(key, "RADAR.TeammateColorB") == 0) { RADAR::TeammateColor.Value.z = iv / 255.f; return; }
        if (lstrcmpA(key, "RADAR.NpcColorR") == 0) { RADAR::NpcColor.Value.x = iv / 255.f; return; }
        if (lstrcmpA(key, "RADAR.NpcColorG") == 0) { RADAR::NpcColor.Value.y = iv / 255.f; return; }
        if (lstrcmpA(key, "RADAR.NpcColorB") == 0) { RADAR::NpcColor.Value.z = iv / 255.f; return; }
        if (lstrcmpA(key, "RADAR.AnimalColorR") == 0) { RADAR::AnimalColor.Value.x = iv / 255.f; return; }
        if (lstrcmpA(key, "RADAR.AnimalColorG") == 0) { RADAR::AnimalColor.Value.y = iv / 255.f; return; }
        if (lstrcmpA(key, "RADAR.AnimalColorB") == 0) { RADAR::AnimalColor.Value.z = iv / 255.f; return; }
        if (lstrcmpA(key, "RADAR.BorderColorR") == 0) { RADAR::BorderColor.Value.x = iv / 255.f; return; }
        if (lstrcmpA(key, "RADAR.BorderColorG") == 0) { RADAR::BorderColor.Value.y = iv / 255.f; return; }
        if (lstrcmpA(key, "RADAR.BorderColorB") == 0) { RADAR::BorderColor.Value.z = iv / 255.f; return; }

        LOAD_BOOL("AIM.Memory", AIMBOT::Memory);
        LOAD_BOOL("AIM.Silent", AIMBOT::Silent);
        LOAD_INT("AIM.FovSize", AIMBOT::FovSize);
        LOAD_FLOAT("AIM.SMOOTHING", AIMBOT::SMOOTHING);
        LOAD_BOOL("AIM.KeepTarget", AIMBOT::KeepTarget);
        LOAD_INT("AIM.BoneSelector", AIMBOT::BoneSelector);
        LOAD_BOOL("AIM.VisibleOnly", AIMBOT::VisibleOnly);
        LOAD_BOOL("AIM.VisibleStrict", AIMBOT::VisibleStrict);
        LOAD_BOOL("AIM.IgnoreTeam", AIMBOT::IgnoreTeam);
        LOAD_BOOL("AIM.IgnoreWounded", AIMBOT::IgnoreWounded);
        LOAD_BOOL("AIM.IgnoreSleepers", AIMBOT::IgnoreSleepers);
        LOAD_FLOAT("AIM.MaxDistanceAim", AIMBOT::MaxDistanceAim);
        LOAD_BOOL("AIM.DynamicFov", AIMBOT::DynamicFov);
        LOAD_BOOL("AIM.WeaponCheck", AIMBOT::WeaponCheck);
        LOAD_INT("AIM.BonePriority", AIMBOT::BonePriority);
        LOAD_FLOAT("AIM.PredictionScale", AIMBOT::PredictionScale);
        LOAD_BOOL("AIM.SpinWrites", AIMBOT::SpinWrites);
        LOAD_BOOL("AIM.FovCircle", AIMBOT::FovCircle);
        LOAD_BOOL("AIM.FovOutline", AIMBOT::FovOutline);
        if (lstrcmpA(key, "AIM.FovColorR") == 0) { AIMBOT::FovColor.Value.x = iv / 255.f; return; }
        if (lstrcmpA(key, "AIM.FovColorG") == 0) { AIMBOT::FovColor.Value.y = iv / 255.f; return; }
        if (lstrcmpA(key, "AIM.FovColorB") == 0) { AIMBOT::FovColor.Value.z = iv / 255.f; return; }
        if (lstrcmpA(key, "AIM.FovColorA") == 0) { AIMBOT::FovColor.Value.w = iv / 255.f; return; }
        LOAD_BOOL("AIM.PredictionIndicator", AIMBOT::PredictionIndicator);
        LOAD_BOOL("AIM.TargetLine", AIMBOT::TargetLine);
        LOAD_BOOL("AIM.TargetText", AIMBOT::TargetText);
        LOAD_BOOL("AIM.TargetLineFromMuzzle", AIMBOT::TargetLineFromMuzzle);
        LOAD_FLOAT("AIM.TargetLineThickness", AIMBOT::TargetLineThickness);
        LOAD_BOOL("AIM.HumanizeEnabled", AIMBOT::HumanizeEnabled);
        LOAD_FLOAT("AIM.JitterAmount", AIMBOT::JitterAmount);
        LOAD_FLOAT("AIM.OvershootAmount", AIMBOT::OvershootAmount);
        LOAD_FLOAT("AIM.SmoothingVariance", AIMBOT::SmoothingVariance);
        LOAD_FLOAT("AIM.MissProbability", AIMBOT::MissProbability);
        if (lstrcmpA(key, "AIM.TargetLineColorR") == 0) { AIMBOT::color::TargetLine.Value.x = iv / 255.f; return; }
        if (lstrcmpA(key, "AIM.TargetLineColorG") == 0) { AIMBOT::color::TargetLine.Value.y = iv / 255.f; return; }
        if (lstrcmpA(key, "AIM.TargetLineColorB") == 0) { AIMBOT::color::TargetLine.Value.z = iv / 255.f; return; }
        if (lstrcmpA(key, "AIM.TargetTextColorR") == 0) { AIMBOT::color::TargetText.Value.x = iv / 255.f; return; }
        if (lstrcmpA(key, "AIM.TargetTextColorG") == 0) { AIMBOT::color::TargetText.Value.y = iv / 255.f; return; }
        if (lstrcmpA(key, "AIM.TargetTextColorB") == 0) { AIMBOT::color::TargetText.Value.z = iv / 255.f; return; }

        LOAD_BOOL("MISC.FovChanger", MISC::FovChanger);
        LOAD_FLOAT("MISC.FovAmount", MISC::FovAmount);
        LOAD_BOOL("MISC.RecoilVariance", MISC::RecoilVariance);
        LOAD_FLOAT("MISC.RecoilFloor", MISC::RecoilFloor);
        LOAD_BOOL("MISC.AntiAnybrain", MISC::AntiAnybrain);
        LOAD_BOOL("MISC.Zoom", MISC::Zoom);
        LOAD_FLOAT("MISC.ZoomAmount", MISC::ZoomAmount);
        LOAD_BOOL("MISC.RecoilEnabled", MISC::RecoilEnabled);
        LOAD_FLOAT("MISC.RecoilModifier", MISC::RecoilModifier);
        LOAD_BOOL("MISC.ChangeBurst", MISC::ChangeBurst);
        LOAD_BOOL("MISC.QuickBurst", MISC::QuickBurst);
        LOAD_FLOAT("MISC.BURSTAMOUNT", MISC::BURSTAMOUNT);
        LOAD_BOOL("MISC.Timechanger", MISC::Timechanger);
        LOAD_INT("MISC.timevalue", MISC::timevalue);
        LOAD_BOOL("MISC.BrightNight", MISC::BrightNight);
        LOAD_BOOL("MISC.Rayleigh", MISC::Rayleigh);
        LOAD_FLOAT("MISC.RayleighVal", MISC::RayleighVal);
        LOAD_BOOL("MISC.Mie", MISC::Mie);
        LOAD_FLOAT("MISC.MieVal", MISC::MieVal);
        LOAD_BOOL("MISC.BrightnessEnv", MISC::BrightnessEnv);
        LOAD_FLOAT("MISC.BrightnessEnvVal", MISC::BrightnessEnvVal);
        LOAD_BOOL("MISC.StarMod", MISC::StarMod);
        LOAD_FLOAT("MISC.StarSize", MISC::StarSize);
        LOAD_FLOAT("MISC.StarBrightness", MISC::StarBrightness);
        LOAD_BOOL("MISC.MeshMod", MISC::MeshMod);
        LOAD_FLOAT("MISC.MeshSize", MISC::MeshSize);
        LOAD_FLOAT("MISC.MeshBrightness", MISC::MeshBrightness);
        LOAD_BOOL("MISC.RemoveTrees", MISC::RemoveTrees);
        LOAD_BOOL("MISC.RemoveGrass", MISC::RemoveGrass);
        LOAD_BOOL("MISC.RemoveWater", MISC::RemoveWater);
        LOAD_BOOL("MISC.RemoveSky", MISC::RemoveSky);
        LOAD_BOOL("MISC.RemoveLayersActive", MISC::RemoveLayersActive);
        LOAD_BOOL("MISC.AspectRatio", MISC::AspectRatio);
        LOAD_FLOAT("MISC.AspectRatioVal", MISC::AspectRatioVal);
        LOAD_BOOL("MISC.TodColors", MISC::TodColors);
        LOAD_BOOL("MISC.TodSunMoon", MISC::TodSunMoon);
        LOAD_BOOL("MISC.TodLight", MISC::TodLight);
        LOAD_BOOL("MISC.TodRay", MISC::TodRay);
        LOAD_BOOL("MISC.TodSky", MISC::TodSky);
        LOAD_BOOL("MISC.TodCloud", MISC::TodCloud);
        LOAD_BOOL("MISC.TodFog", MISC::TodFog);
        LOAD_BOOL("MISC.TodAmbient", MISC::TodAmbient);
        LOAD_FLOAT("MISC.lightIntensity", MISC::lightIntensity);
        LOAD_FLOAT("MISC.ambientMultiplier", MISC::ambientMultiplier);
        LOAD_FLOAT("MISC.AmbientSaturation", MISC::AmbientSaturation);
        LOAD_BOOL("MISC.ShowServerTime", MISC::ShowServerTime);
        LOAD_INT("MISC.CrosshairStyle", MISC::CrosshairStyle);
        LOAD_FLOAT("MISC.CrosshairSize", MISC::CrosshairSize);
        if (lstrcmpA(key, "MISC.CrosshairColorR") == 0) { MISC::CrosshairColor.Value.x = iv / 255.f; return; }
        if (lstrcmpA(key, "MISC.CrosshairColorG") == 0) { MISC::CrosshairColor.Value.y = iv / 255.f; return; }
        if (lstrcmpA(key, "MISC.CrosshairColorB") == 0) { MISC::CrosshairColor.Value.z = iv / 255.f; return; }
        LOAD_BOOL("MISC.DebugCamera", MISC::DebugCamera);
        LOAD_BOOL("MISC.AdminFlags", MISC::AdminFlags);
        LOAD_BOOL("MISC.CombatMode", MISC::CombatMode);
        LOAD_BOOL("MISC.DrawFlags", MISC::DrawFlags);
        LOAD_BOOL("MISC.ShowEspRangeOverlay", MISC::ShowEspRangeOverlay);
        LOAD_BOOL("MISC.SilentShot", MISC::SilentShot);
        LOAD_BOOL("MISC.usetouchUnderWater", MISC::usetouchUnderWater);
        LOAD_BOOL("MISC.MiniGunMagicBullet", MISC::MiniGunMagicBullet);
        LOAD_BOOL("MISC.NoMeleeCoolDown", MISC::NoMeleeCoolDown);
        LOAD_BOOL("MISC.NoSpread", MISC::NoSpread);
        LOAD_BOOL("MISC.Automatic", MISC::Automatic);
        LOAD_BOOL("MISC.Aimsway", MISC::Aimsway);
        LOAD_BOOL("MISC.NoPunch", MISC::NoPunch);
        LOAD_BOOL("MISC.RapidFire", MISC::RapidFire);
        LOAD_BOOL("MISC.SuperMelee", MISC::SuperMelee);
        LOAD_BOOL("MISC.InstantBow", MISC::InstantBow);
        LOAD_BOOL("MISC.HitSound", MISC::HitSound);
        LOAD_BOOL("MISC.InstantEoka", MISC::InstantEoka);
        LOAD_BOOL("MISC.FastBow", MISC::FastBow);
        LOAD_BOOL("MISC.NoPullback", MISC::NoPullback);
        LOAD_BOOL("MISC.InstantCompound", MISC::InstantCompound);
        LOAD_BOOL("MISC.Longhand", MISC::Longhand);
        LOAD_BOOL("MISC.NoHeavySlow", MISC::NoHeavySlow);
        LOAD_BOOL("MISC.NoAnim", MISC::NoAnim);
        LOAD_BOOL("MISC.NoBob", MISC::NoBob);
        LOAD_BOOL("MISC.NoSway", MISC::NoSway);
        LOAD_BOOL("MISC.NoLower", MISC::NoLower);
        LOAD_BOOL("MISC.AlwaysSprint", MISC::AlwaysSprint);
        LOAD_BOOL("MISC.omniSprint", MISC::omniSprint);
        LOAD_BOOL("MISC.JumpShot", MISC::JumpShot);
        LOAD_BOOL("MISC.WalkonWalls", MISC::WalkonWalls);
        LOAD_BOOL("MISC.NoFall", MISC::NoFall);
        LOAD_BOOL("MISC.ShootInAir", MISC::ShootInAir);
        LOAD_BOOL("MISC.ReducedGravity", MISC::ReducedGravity);
        LOAD_FLOAT("MISC.GravityValue", MISC::GravityValue);
        LOAD_BOOL("MISC.MoonJump", MISC::MoonJump);
        LOAD_BOOL("MISC.Spinbot", MISC::Spinbot);
        LOAD_BOOL("MISC.mincoptershoot", MISC::mincoptershoot);
        LOAD_BOOL("MISC.AimWhileHeavy", MISC::AimWhileHeavy);
        LOAD_BOOL("MISC.FASTHORSE", MISC::FASTHORSE);
        LOAD_BOOL("MISC.Fly", MISC::Fly);
        LOAD_FLOAT("MISC.FlySpeed", MISC::FlySpeed);
        LOAD_BOOL("MISC.FlySafeMode", MISC::FlySafeMode);
        LOAD_BOOL("MISC.HighJump", MISC::HighJump);
        LOAD_FLOAT("MISC.HighJumpAngle", MISC::HighJumpAngle);
        LOAD_INT("MISC.FlyKey", MISC::FlyKey);
        LOAD_BOOL("MISC.SpeedHack", MISC::SpeedHack);
        LOAD_INT("MISC.SpeedKey", MISC::SpeedKey);
        LOAD_FLOAT("MISC.SpeedValue", MISC::SpeedValue);
        LOAD_BOOL("MISC.Spiderman", MISC::Spiderman);
        LOAD_BOOL("MISC.WalkOnWater", MISC::WalkOnWater);
        LOAD_BOOL("MISC.InfJump", MISC::InfJump);
        LOAD_BOOL("MISC.NoWearOverlay", MISC::NoWearOverlay);
        LOAD_FLOAT("MISC.SpreadScale", MISC::SpreadScale);
        LOAD_FLOAT("MISC.FireRateScale", MISC::FireRateScale);
        LOAD_BOOL("SETTINGS.AdminFlag", SETTINGS::AdminFlag);
        LOAD_BOOL("SETTINGS.BattleMode", SETTINGS::BattleMode);
        LOAD_FLOAT("SETTINGS.ROTSPEED", SETTINGS::ROTSPEED);
        LOAD_INT("SETTINGS.Language", SETTINGS::Language);
        LOAD_BOOL("BATTLE.Players", BATTLE::Players);
        LOAD_BOOL("BATTLE.Aimbot", BATTLE::Aimbot);
        LOAD_BOOL("BATTLE.World", BATTLE::World);
        LOAD_BOOL("BATTLE.NPC", BATTLE::NPC);
        LOAD_BOOL("BATTLE.Animals", BATTLE::Animals);
        LOAD_BOOL("BATTLE.Radar", BATTLE::Radar);

        #undef LOAD_BOOL
        #undef LOAD_INT
        #undef LOAD_FLOAT
    }

    inline void ApplyLegitDefaults() {
        ESP::ESPEnabled = true;
        ESP::Box = true; ESP::CornerBox = false; ESP::FilledBox = false;
        ESP::Name = true; ESP::Distance = true; ESP::HealthBar = true;
        ESP::Weapon = false; ESP::Skeleton = false; ESP::HeadCircle = false;
        ESP::SnapLines = false; ESP::OFFArrows = false;         ESP::VisCheck = true;
        ESP::TeamID = true; ESP::hotbar_text = false;
        ESP::AmmoBar = false; ESP::ReloadBar = false;
        ESP::RemoveSleepers = true; ESP::RemoveWounded = true;
        ESP::draw_distance = 200.f;
        AIMBOT::Memory = false; MISC::RecoilModifier = 0.f;
        MISC::NoSpread = false; MISC::Automatic = false;
        RADAR::Enabled = true;
        WORLD::Stone = true; WORLD::Metal = true; WORLD::Sulfer = true; WORLD::Hemp = true;
        WORLD::Stash = true; WORLD::DroppedItem = true; WORLD::Backpack = true;
        WORLD::Turret = true; WORLD::ShotGunTrap = true;
        WORLD::draw_distance = 250.f;
        NPC_ESP::Enabled = true; NPC_ESP::Name = true; NPC_ESP::Distance = true;
        NPC_ESP::HealthBar = false; NPC_ESP::Skeleton = false;
        NPC_ESP::HeldItem = false; NPC_ESP::SnapLines = false;
        NPC_ESP::draw_distance = 200.f;
        ANIMAL_ESP::Enabled = true;
        ANIMAL_ESP::Box = false; ANIMAL_ESP::Name = true; ANIMAL_ESP::Distance = true;
        ANIMAL_ESP::draw_distance = 150.f;
        MISC::FovChanger = false; MISC::Zoom = false;
        SETTINGS::BattleMode = false;
    }

    inline bool Load() {
        ApplyLegitDefaults();

        HANDLE h = CreateFileA(GetDefaultConfigPath(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE) {
            SetStatus("No config found — defaults loaded");
            return false;
        }

        DWORD fileSize = GetFileSize(h, NULL);
        if (fileSize == 0 || fileSize > 1024 * 64) { CloseHandle(h); SetStatus("Config invalid — defaults loaded"); return false; }

        char* data = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, fileSize + 1);
        if (!data) { CloseHandle(h); SetStatus("Load failed: memory error"); return false; }

        DWORD bytesRead;
        ReadFile(h, data, fileSize, &bytesRead, NULL);
        CloseHandle(h);
        data[bytesRead] = 0;

        char* lineStart = data;
        for (DWORD i = 0; i <= bytesRead; i++) {
            if (data[i] == '\n' || data[i] == 0) {
                data[i] = 0;
                if (lineStart < data + i) ParseLine(lineStart);
                lineStart = data + i + 1;
            }
        }
        HeapFree(GetProcessHeap(), 0, data);

        if (ESP::draw_distance < 1.f) ESP::draw_distance = 200.f;
        if (WORLD::draw_distance < 1.f) WORLD::draw_distance = 250.f;
        if (NPC_ESP::draw_distance < 1.f) NPC_ESP::draw_distance = 200.f;
        if (ANIMAL_ESP::draw_distance < 1.f) ANIMAL_ESP::draw_distance = 150.f;

        SetStatus("Config loaded");
        return true;
    }
}

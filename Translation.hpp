#pragma once
#include <string>
#include <unordered_map>
#include "offsets.hpp"

// French translation pairs — key=English, value=French.
// Stored as static string literals so c_str() never dangles.
struct TranslationPair { const char* en; const char* fr; };

inline const TranslationPair g_TranslationPairs[] = {
    // === Pages ===
    {"Dashboard","Tableau de bord"},{"Combat","Combat"},{"Player ESP","ESP Joueur"},{"NPC ESP","ESP PNJ"},
    {"Animal ESP","ESP Animaux"},{"World ESP","ESP Monde"},{"Radar","Radar"},{"Visuals","Visuels"},
    {"Utility","Utilitaires"},{"Settings","Paramètres"},
    {"NPCs & Animals","PNJs & Animaux"},{"Visuals & Radar","Visuels & Radar"},
    {"Status","Statut"},{"Favorites","Favoris"},{"Quick Presets","Préréglages Rapides"},
    {"Aimbot","Aimbot"},{"Weapon Mods","Mods Armes"},{"Additional Tools","Outils Supplémentaires"},
    {"Movement","Mouvement"},

    // === Combat toggles ===
    {"Memory Aimbot","Aimbot Mémoire"},
    {"Recoil Modifier","Modificateur Recul"},

    // === Player ESP ===
    {"Box","Boîte"},{"Corner Box","Boîte Coin"},{"Filled Box","Boîte Remplie"},
    {"Corners","Coins"},{"Filled","Remplie"},
    {"Skeleton","Squelette"},{"Head Circle","Cercle Tête"},
    {"Snaplines","Lignes"},{"Snapline Mode","Mode Ligne"},
    {"OFF Arrows","Flèches Hors-Champ"},{"Name","Nom"},{"Distance","Distance"},
    {"Held Item","Arme"},{"Weapon","Arme"},
    {"Team ID","ID Équipe"},
    {"Hotbar Text","Barre Rapide"},{"Flag Check","Vérif. Drapeaux"},
    {"Remove Wounded","Retirer Blessés"},{"Remove Sleepers","Retirer Dormeurs"},
    {"Remove Team","Retirer Équipe"},
    {"Highlight Wounded","Surligner Blessés"},{"Highlight Sleepers","Surligner Dormeurs"},
    {"Draw Distance","Distance d'Affichage"},
    {"Advanced Distances","Distances Avancées"},
    {"Box Dist","Dist Boîte"},{"Skeleton Dist","Dist Squelette"},
    {"Name Dist","Dist Nom"},{"Weapon Dist","Dist Arme"},
    {"Snapline Dist","Dist Ligne"},
    {"HeadCircle Dist","Dist Cercle"},{"OFF Arrow Dist","Dist Flèche"},
    {"Visible","Visible"},{"Invisible","Invisible"},
    {"Visible Color","Couleur Visible"},{"Invisible Color","Couleur Invisible"},
    {"Ignore Team","Ignorer Équipe"},{"Ignore Wounded","Ignorer Blessés"},
    {"Ignore Sleepers","Ignorer Dormeurs"},

    // === Radar ===
    {"Enable Radar","Activer Radar"},{"Radar Settings","Paramètres Radar"},
    {"Size","Taille"},{"Scale","Échelle"},{"Dot Size","Taille Point"},
    {"Position","Position"},{"Players","Joueurs"},{"NPCs","PNJs"},{"Animals","Animaux"},
    {"Grid","Grille"},{"Rotate","Rotation"},{"Background Opacity","Opacité Fond"},
    {"Player Color","Couleur Joueur"},{"NPC Color","Couleur PNJ"},{"Animal Color","Couleur Animal"},
    {"Border Color","Couleur Bordure"},{"Player Dot","Point Joueur"},{"NPC Dot","Point PNJ"},
    {"Animal Dot","Point Animal"},{"Radar Border","Bordure Radar"},

    // === Off/Bottom/Middle/Top ===
    {"Off","Arrêt"},{"Bottom","Bas"},{"Middle","Milieu"},{"Top","Haut"},

    // === NPC ===
    {"Enable NPC ESP","Activer ESP PNJ"},{"NPC Box","Boîte PNJ"},{"NPC Name","Nom PNJ"},
    {"NPC Distance","Distance PNJ"},{"NPC Skeleton","Squelette PNJ"},
    {"NPC Held Item","Arme PNJ"},{"NPC Snaplines","Lignes PNJ"},
    {"NPC Box Dist","Dist Boîte PNJ"},{"NPC Name Dist","Dist Nom PNJ"},
    {"NPC DistText Dist","Dist Texte PNJ"},
    {"NPC Skeleton Dist","Dist Squelette PNJ"},{"NPC HeldItem Dist","Dist Arme PNJ"},
    {"NPC Snapline Dist","Dist Ligne PNJ"},
    {"Scientist","Scientifique"},{"Tunnel Dweller","Habitant Tunnel"},
    {"Underwater Dweller","Habitant Sous-Marin"},{"NPC","PNJ"},
    {"NPC Types","Types PNJ"},{"NPC Visuals","Visuels PNJ"},

    // === Animal ===
    {"Enable Animal ESP","Activer ESP Animaux"},{"Animal Types","Types d'Animaux"},
    {"Animal Box","Boîte Animal"},{"Animal Name","Nom Animal"},    {"Animal Distance","Distance Animal"},
    {"Animal Snaplines","Lignes Animal"},
    {"Animal Box Dist","Dist Boîte Animal"},{"Animal Name Dist","Dist Nom Animal"},
    {"Animal DistText Dist","Dist Texte Animal"},
    {"Animal Snapline Dist","Dist Ligne Animal"},
    {"Bear","Ours"},{"Wolf","Loup"},{"Boar","Sanglier"},{"Stag","Cerf"},
    {"Chicken","Poulet"},{"Horse","Cheval"},{"Polar Bear","Ours Polaire"},
    {"Panther","Panthère"},{"Tiger","Tigre"},{"Snake","Serpent"},{"Animal","Animal"},
    {"Animal Visuals","Visuels Animaux"},

    // === World ===
    {"Stone Ore","Minerai Pierre"},{"Metal Ore","Minerai Métal"},{"Sulfur Ore","Minerai Soufre"},
    {"Sulfer Ore","Minerai Soufre"},
    {"Hemp","Chanvre"},{"Stash","Cache"},{"Body Bag","Sac Mortuaire"},
    {"Backpack","Sac à Dos"},{"Dropped Items","Objets au Sol"},
    {"Auto Turret","Tourelle Auto"},{"Shotgun Trap","Piège Fusil"},
    {"MiniCopter","MiniHélico"},{"Bradley APC","Bradley APC"},
    {"Show Distance","Afficher Distance"},{"Global World Draw","Affichage Monde Global"},
    {"Corpse","Cadavre"},{"Resources","Ressources"},{"Loot","Butin"},
    {"Traps & Vehicles","Pièges & Véhicules"},{"Display","Affichage"},

    // === Visuals ===
    {"Camera","Caméra"},{"World Visuals","Visuels Monde"},
    {"FOV Changer","Changeur FOV"},{"FOV","FOV"},{"Zoom","Zoom"},
    {"Zoom Amount","Quantité Zoom"},{"Crosshair","Réticule"},{"Crosshair Size","Taille Réticule"},
    {"Crosshair Color","Couleur Réticule"},{"Server Time","Heure Serveur"},
    {"Screen Info","Info Écran"},{"Time Changer","Changeur Temps"},
    {"Time","Heure"},{"Bright Night","Nuit Claire"},
    {"Light Intensity","Intensité Lumière"},{"Ambient","Ambiance"},

    // === Utility ===
    {"Debug Camera","Caméra Libre"},{"Combat Mode","Mode Combat"},
    {"Admin Flag","Drapeau Admin"},{"VSync","VSync"},{"Battle Mode","Mode Bataille"},
    {"Tools","Outils"},{"Admin","Admin"},

    // === Settings ===
    {"Save Config","Sauver Config"},{"Load Config","Charger Config"},
    {"Config","Configuration"},{"Path","Chemin"},
    {"Save As","Enregistrer Sous"},{"Load Name","Charger Nom"},
    {"Delete","Supprimer"},{"Rename","Renommer"},{"Rename to...","Renommer en..."},
    {"Export","Exporter"},{"Import","Importer"},
    {"Config name...","Nom config..."},
    {"Config Saved!","Config Sauvée!"},{"Config Loaded!","Config Chargée!"},
    {"Create Config","Créer Config"},{"Rename Config","Renommer Config"},
    {"Delete Config","Supprimer Config"},{"Export Config","Exporter Config"},
    {"Import Config","Importer Config"},
    {"Presets","Préréglages"},{"Keybinds","Raccourcis"},
    {"PvP Setup","Config PvP"},{"Farm Setup","Config Farm"},
    {"Explore","Exploration"},{"StreamerSafe","StreamerSafe"},
    {"Appearance","Apparence"},{"Background FX","Effets Fond"},
    {"Misc","Divers"},{"Colors","Couleurs"},{"About","À Propos"},
    {"Theme","Thème"},{"Accent","Accent"},{"UI Scale","Échelle UI"},
    {"Menu Opacity","Opacité Menu"},{"Border Glow","Lueur Bordure"},
    {"Animation Speed","Vitesse Animation"},{"Compact Mode","Mode Compact"},
    {"Advanced Mode","Mode Avancé"},{"Disable Animations","Désactiver Animations"},
    {"Performance Mode","Mode Performance"},
    {"Animated Background","Fond Animé"},{"Matrix Particles","Particules Matrix"},
    {"Lightning FX","Effets Éclairs"},{"FX Intensity","Intensité FX"},
    {"Matrix Density","Densité Matrix"},{"BG Opacity","Opacité Fond"},
    {"Tooltips","Infobulles"},{"Show Watermark","Filigrane"},
    {"Show FPS","Afficher FPS"},{"Confirm Before Reset","Confirmer Réinitialisation"},
    {"ESP Style","Style ESP"},{"Rotation Speed","Vitesse Rotation"},
    {"Username BG","Fond Nom"},{"Distance BG","Fond Distance"},

    // === Color labels ===
    {"Anim Box","Boîte Animal"},{"Anim Name","Nom Animal"},{"Anim Dist","Dist Animal"},
    {"Player","Joueur"},{"NPC","PNJ"},
    {"Stone","Pierre"},{"Metal","Métal"},{"Sulfur","Soufre"},
    {"Turret","Tourelle"},{"DropItem","Objet Sol"},
    {"Bradley","Bradley"},{"Shotgun","Fusil"},

    // === Misc ===
    {"Saved","Sauvé"},{"Loaded","Chargé"},{"Reset","Réinitialisé"},
    {"Save","Sauver"},{"Load","Charger"},{"Panic","Panique"},
    {"Clear All","Tout Effacer"},{"Conflicts","Conflits"},{"Conflict","Conflit"},
    {"Search binds...","Rechercher..."},{"Search settings...","Rechercher paramètres..."},
    {"Search","Recherche"},{"All","Tout"},{"Global","Global"},
    {"Page","Page"},{"Key","Touche"},{"Mode","Mode"},{"Feature","Fonction"},
    {"Toggle","Bascule"},{"Hold","Maintenir"},{"Always","Toujours"},{"Disabled","Désactivé"},
    {"None","Aucun"},{"Clear","Effacer"},{"Info","Info"},{"Filters","Filtres"},
    {"Yes","Oui"},{"No","Non"},

    // === Warnings ===
    {"Memory aimbot - moderate detection risk","Aimbot mémoire - risque détection modéré"},
    {"No vision/occlusion check - uses player flags only","Pas de vérification visuelle - utilise drapeaux uniquement"},
    {"Spin writes may desync on high-latency servers","Écritures rapides peuvent désynchroniser sur serveurs haute latence"},
    {"Prediction may overshoot on jittery targets","La prédiction peut dépasser sur cibles saccadées"},
    {"No weapon tilt applied - writes directly to PlayerInput","Pas d'inclinaison arme - écrit directement dans PlayerInput"},
    {"Force burst mode on semi-auto weapons","Force le mode rafale sur armes semi-auto"},
    {"Rapid burst interval override","Remplacement intervalle rafale rapide"},
    {"NoSpread forces aimcone to zero — detectable","NoSpread met l'aimcone à zéro — détectable"},
    {"Converts semi-auto to full-auto","Convertit semi-auto en full-auto"},
    {"Mag dump at 50 rounds/sec","Vidage chargeur à 50 balles/sec"},
    {"Melee damage x4 range x2","Dégâts mêlée x4 portée x2"},
    {"Instant arrow charge + release","Charge et tir flèche instantané"},
    {"Silent muzzle flash + sound","Flash et son de bouche silencieux"},
    {"Minigun magic bullet override","Balle magique minigun"},
    {"Chams for wielded weapons","Chams pour armes équipées"},
    {"Touch underwater flag override","Drapeau toucher sous l'eau"},
    {"Bullet velocity x1.5","Vitesse balle x1.5"},
    {"Zero melee cooldown flag","Délai mêlée zéro"},
    {"Removes viewmodel punch/bob","Retire le recul camera"},
    {"Play hitmarker sound on damage event","Son d'impact sur événement dégâts"},
    {"Removes background behind ESP text","Retire le fond derrière le texte ESP"},
    {"Unrestricted sprint directions","Directions sprint illimitées"},
    {"Jump shot accuracy - server-validated","Précision tir en saut - validé serveur"},
    {"Climb steep surfaces","Grimpe surfaces raides"},
    {"Horse speed bypass","Contournement vitesse cheval"},
    {"Shoot while piloting minicopter","Tirer en pilotant minihélico"},
    {"Some servers validate FOV range","Certains serveurs valident la plage FOV"},
    {"FOV manipulation while key held","Manipulation FOV pendant touche maintenue"},
    {"Server validates time of day","Le serveur valide l'heure du jour"},
    {"Detached camera mode (no noclip writes)","Mode caméra détachée (pas d'écriture noclip)"},
    {"Grants unauthorized admin privileges","Accorde privilèges admin non autorisés"},
    {"Local admin on modded servers.","Admin local sur serveurs moddés."},

    // === Overlay ===
    {"COMBAT MODE","MODE COMBAT"},{"Player ESP Range","Portée ESP Joueur"},
    {"No favorites yet.","Pas de favoris pour le moment."},
    {"INSERT: Toggle Menu  |  END: Exit","INSERT: Menu  |  FIN: Quitter"},
    {"Reset all settings to defaults?","Reinitialiser tous les parametres?"},
    {"Search Results","Resultats Recherche"},
    {"*Unsaved","*Non Sauve"},{"Bind: %s","Raccourci: %s"},

    // === Aimbot / Combat Labels ===
    {"Smoothing","Lissage"},{"FOV Size","Taille FOV"},{"Bone Priority","Priorite Os"},
    {"Head","Tete"},{"Neck","Cou"},{"Chest","Poitrine"},{"Spine","Colonne"},{"Pelvis","Bassin"},
    {"Closest to Crosshair","Plus Proche Reticule"},{"Smart","Intelligent"},{"Aim Bone","Os de Visee"},
    {"Prediction","Prediction"},{"Line Thickness","Epaisseur Ligne"},
    {"Line Color","Couleur Ligne"},{"Target Icon Color","Couleur Icone Cible"},
    {"Recoil Mod %","Mod Recul %"},

    // === Player ESP advanced ===
    {"Box Thickness","Epaisseur Boite"},{"Skeleton Thickness","Epaisseur Squelette"},
    {"Snapline Thickness","Epaisseur Ligne"},
    {"Flag","Drapeau"},{"Hybrid","Hybride"},{"Strict","Strict"},
    {"Vis Mode","Mode Visibilite"},{"Vis Samples","Echantillons Vis"},
    {"Vis Hold (ms)","Maintien Vis (ms)"},{"Vis Max Stale (ms)","Max Perime Vis (ms)"},
    {"Max Stale: bypass filter when raw flag unchanged this long.","Max Perime: contourne filtre quand drapeau inchange trop longtemps."},
    {"Ghost Duration (ms)","Duree Fantome (ms)"},
    {"Box Dist","Dist Boite"},{"Skeleton Dist","Dist Squelette"},
    {"HeadCircle Dist","Dist Cercle"},{"OFF Arrow Dist","Dist Fleche"},
    {"Active:","Actif:"},{"Ammo: ","Munit: "},
    {"-- Wear --","-- Vetements --"},{"-- Belt --","-- Ceinture --"},
    {" (ghost)"," (fantome)"},{"Player","Joueur"},

    // === NPC ESP extra ===
    {"NPC Name Dist","Dist Nom PNJ"},{"NPC DistText Dist","Dist Texte PNJ"},
    {"NPC Skeleton Dist","Dist Squelette PNJ"},{"NPC Held Dist","Dist Arme PNJ"},
    {"NPC Snapline Dist","Dist Ligne PNJ"},

    // === Animal ESP extra ===
    {"Animal Box Dist","Dist Boite Animal"},{"Animal Name Dist","Dist Nom Animal"},
    {"Animal DistText Dist","Dist Texte Animal"},{"Animal HP Dist","Dist Sante Animal"},

    // === World ESP (43 entities) ===
    {"Stone Pkup","Ramassage Pierre"},{"Metal Pkup","Ramassage Metal"},
    {"Sulfur Pkup","Ramassage Soufre"},{"Wood Pkup","Ramassage Bois"},
    {"Sam Site","Site SAM"},{"Bear Trap","Piege a Ours"},{"Landmine","Mine Terrestre"},
    {"Flame Turret","Tourelle Flamme"},{"Ballista","Baliste"},{"Catapult","Catapulte"},
    {"Locker","Casier"},{"Sleeping Bag","Sac de Couchage"},{"Bed","Lit"},
    {"Tool Cupboard","Armoire a Outils"},{"Vending","Distributeur"},{"Workbench","Etabli"},
    {"Large Storage","Grand Stockage"},{"Ladder","Echelle"},{"Generator","Generateur"},
    {"Battery","Batterie"},{"Barrel","Baril"},{"Fuel Barrel","Baril Carburant"},
    {"Crate","Caisse"},{"Mil Crate","Caisse Militaire"},{"Elite Crate","Caisse Elite"},
    {"Locked Crate","Caisse Verrouillee"},{"Med Crate","Caisse Medicale"},{"Food Crate","Caisse Nourriture"},
    {"Rowboat","Barque"},{"RHIB","RHIB"},{"Kayak","Kayak"},{"Tugboat","Remorqueur"},
    {"Submarine","Sous-Marin"},{"Transport Heli","Helico Transport"},
    {"Attack Heli","Helico Attaque"},{"Balloon","Montgolfiere"},
    {"Motorbike","Moto"},{"Sidecar","Side-car"},{"Trike","Trike"},
    {"Bicycle","Velo"},{"Snowmobile","Motoneige"},{"Shark","Requin"},{"Cargo Ship","Navire Cargo"},{"Supply Drop","Largage"},

    // === Radar ===
    {"Top Left","Haut Gauche"},{"Top Right","Haut Droite"},
    {"Bottom Left","Bas Gauche"},{"Bottom Right","Bas Droite"},

    // === Dashboard ===
    {"FPS: %.0f","FPS: %.0f"},{"Entities: %d","Entites: %d"},
    {"Players: %d  |  NPCs: %d  |  Animals: %d","Joueurs: %d  |  PNJs: %d  |  Animaux: %d"},
    {"Active Features: %d","Fonctions Actives: %d"},
    {"Click the star icon on any setting to add it here.","Cliquez l'icone etoile sur un reglage pour l'ajouter ici."},
    {"The BeaZt -- Private Legit","The BeaZt -- Prive Legitime"},
    {"Red text = HIGH ban risk","Texte rouge = Risque BAN eleve"},
    {"Orange text = Moderate risk","Texte orange = Risque modere"},
    {"White text = Safe (ESP/read-only)","Texte blanc = Sur (ESP/lecture seule)"},

    // === UI Misc ===
    {"Press any key or mouse button...","Appuyez une touche..."},
    {"Mode: %s","Mode: %s"},{"Saved Configs","Configs Sauvees"},
    {"Config name...","Nom config..."},{"Path: %s","Chemin: %s"},
    {"Quick Actions","Actions Rapides"},{"BATTLE MODE","MODE BATAILLE"},
    {"DEBUG CAM EXIT IN: %.0fs","SORTIE CAM DEBUG: %.0fs"},

    // === Settings ===
    {"85%","85%"},{"90%","90%"},{"100%","100%"},{"110%","110%"},{"120%","120%"},
    {"The BeaZt","The BeaZt"},{"Private Legit Cheat","Cheat Prive Legitime"},
    {"June 2025  |  v1.0","Juin 2025  |  v1.0"},

    // === Keybind Pages ===
    {"Global","Global"},{"Combat","Combat"},{"NPC","PNJ"},{"Animal","Animal"},
    {"World","Monde"},{"Visuals","Visuels"},{"Utility","Utilitaire"},{"Other","Autre"},

    // === Bone combo ===
    {"l_hip","Hanche G"},{"l_knee","Genou G"},{"l_foot","Pied G"},
    {"r_hip","Hanche D"},{"r_knee","Genou D"},{"r_foot","Pied D"},
    {"spine1","Colonne 1"},{"spine2","Colonne 2"},{"spine3","Colonne 3"},{"spine4","Colonne 4"},
    {"l_upperarm","Bras G"},{"l_forearm","Av-Bras G"},{"l_hand","Main G"},
    {"r_upperarm","Bras D"},{"r_forearm","Av-Bras D"},{"r_hand","Main D"},
    {"jaw","Machoire"},{"l_eye","Oeil G"},{"r_eye","Oeil D"},
};

inline std::unordered_map<std::string, const char*> g_TranslationMap;

inline void InitTranslations() {
    g_TranslationMap.clear();
    for (const auto& p : g_TranslationPairs) {
        g_TranslationMap[p.en] = p.fr;  // French string literals — c_str() stays valid forever
    }
}

inline const char* TR(const char* key) {
    if (!key || SETTINGS::Language == 0) return key;
    auto it = g_TranslationMap.find(key);
    return (it != g_TranslationMap.end()) ? it->second : key;
}

inline const char* TR(const std::string& key) { return TR(key.c_str()); }

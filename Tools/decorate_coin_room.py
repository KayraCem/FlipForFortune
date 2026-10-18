# Flip for Fortune - CoinRoom'u susler ve masaya tiklanabilir esyalar koyar.
# Calistirma: Unreal Editor > Tools > Execute Python Script > bu dosya
# (Tekrar calistirilabilir: once eski Props/Decor/Interactables klasorlerini siler.)
import math
import unreal

ROOT = "/Game/FlipForFortune"
MAP_PATH = ROOT + "/Maps/CoinRoom"
MAT_DIR = ROOT + "/Materials"
COIN_DIR = ROOT + "/Coins"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

CUBE = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
CYL = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
SPH = unreal.load_asset("/Engine/BasicShapes/Sphere.Sphere")
CONE = unreal.load_asset("/Engine/BasicShapes/Cone.Cone")

KEEP = unreal.AttachmentRule.KEEP_WORLD
MOVABLE = unreal.ComponentMobility.MOVABLE
TABLE_Z = 80.0


def V(x, y, z):
    return unreal.Vector(x, y, z)


def R(roll=0.0, pitch=0.0, yaw=0.0):
    return unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw)



def enum_value(enum_cls, name):
    """Enum degerini isimden bul (DUST / Dust / GOLD_RING / GoldRing hepsi olur)."""
    key = name.replace("_", "").upper()
    for attr in dir(enum_cls):
        if attr.startswith("_"):
            continue
        if attr.replace("_", "").upper() == key:
            return getattr(enum_cls, attr)
    return None


def import_texture(rel_file, dest_dir, name):
    """Proje klasorundeki PNG'yi Unreal dokusu olarak ice aktar."""
    try:
        f = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()) + rel_file
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", f)
        task.set_editor_property("destination_path", dest_dir)
        task.set_editor_property("destination_name", name)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("automated", True)
        task.set_editor_property("save", True)
        asset_tools.import_asset_tasks([task])
        return unreal.load_asset(f"{dest_dir}/{name}")
    except Exception as e:
        unreal.log_warning(f"Doku ice aktarilamadi {rel_file}: {e}")
        return None

# ---------------------------------------------------------------- materyaller
def make_material(name, rgb, metallic=0.0, roughness=0.5, emissive=None, opacity=None):
    path = f"{MAT_DIR}/{name}"
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    mat = asset_tools.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    if opacity is not None:
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    col = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 0)
    col.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    met = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 200)
    met.set_editor_property("r", metallic)
    mel.connect_material_property(met, "", unreal.MaterialProperty.MP_METALLIC)
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 300)
    rough.set_editor_property("r", roughness)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if emissive is not None:
        em = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 450)
        em.set_editor_property("constant", unreal.LinearColor(emissive[0], emissive[1], emissive[2], 1.0))
        mel.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if opacity is not None:
        op = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 600)
        op.set_editor_property("r", opacity)
        mel.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    return mat



def make_fx_glow_material():
    """Inis efektleri icin parlayan (unlit) materyal: 'Color' ve 'Glow' parametreleri."""
    path = f"{MAT_DIR}/M_FX_Glow"
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    mat = asset_tools.create_asset("M_FX_Glow", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    col = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -600, 0)
    col.set_editor_property("parameter_name", "Color")
    col.set_editor_property("default_value", unreal.LinearColor(1.0, 0.8, 0.3, 1.0))
    glow = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -600, 200)
    glow.set_editor_property("parameter_name", "Glow")
    glow.set_editor_property("default_value", 6.0)
    mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 100)
    mel.connect_material_expressions(col, "", mul, "A")
    mel.connect_material_expressions(glow, "", mul, "B")
    mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    return mat


make_fx_glow_material()

M = {
    "gold": make_material("M_CoinGold", (1.0, 0.72, 0.25), 1.0, 0.25),
    "bronze": make_material("M_CoinBronze", (0.62, 0.36, 0.18), 1.0, 0.35),
    "silver": make_material("M_CoinSilver", (0.85, 0.86, 0.9), 1.0, 0.2),
    "clover": make_material("M_CoinClover", (0.15, 0.7, 0.3), 1.0, 0.3),
    "phoenix": make_material("M_CoinPhoenix", (1.0, 0.3, 0.05), 1.0, 0.3, emissive=(0.6, 0.12, 0.0)),
    "chest": make_material("M_ChestWood", (0.16, 0.08, 0.035), 0.0, 0.6),
    "iron": make_material("M_Iron", (0.08, 0.08, 0.09), 1.0, 0.45),
    "leather_red": make_material("M_LeatherRed", (0.32, 0.03, 0.03), 0.0, 0.55),
    "leather_green": make_material("M_LeatherGreen", (0.04, 0.16, 0.08), 0.0, 0.6),
    "paper": make_material("M_Paper", (0.85, 0.8, 0.65), 0.0, 0.9),
    "red_paint": make_material("M_RedPaint", (0.6, 0.04, 0.04), 0.2, 0.3),
    "glass": make_material("M_Glass", (0.8, 0.9, 1.0), 0.0, 0.05, opacity=0.2),
    "cap_blue": make_material("M_CapsuleBlue", (0.1, 0.3, 1.0), 0.0, 0.3),
    "cap_pink": make_material("M_CapsulePink", (1.0, 0.25, 0.6), 0.0, 0.3),
    "cap_yellow": make_material("M_CapsuleYellow", (1.0, 0.85, 0.1), 0.0, 0.3),
    "dark": make_material("M_DarkMetal", (0.02, 0.02, 0.02), 0.5, 0.5),
    "candle": make_material("M_Candle", (0.9, 0.85, 0.7), 0.0, 0.7),
    "flame": make_material("M_Flame", (1.0, 0.6, 0.1), 0.0, 1.0, emissive=(25.0, 9.0, 1.0)),
    "lampshade": make_material("M_LampShade", (0.9, 0.7, 0.4), 0.0, 0.8, emissive=(3.0, 1.8, 0.7)),
    "rug": make_material("M_Rug", (0.24, 0.03, 0.05), 0.0, 1.0),
    "rug_border": make_material("M_RugBorder", (0.55, 0.38, 0.1), 0.0, 1.0),
    "panel": make_material("M_WoodPanel", (0.13, 0.07, 0.035), 0.0, 0.5),
    "canvas_a": make_material("M_PaintingA", (0.05, 0.15, 0.12), 0.0, 0.9),
    "canvas_b": make_material("M_PaintingB", (0.12, 0.08, 0.2), 0.0, 0.9),
    "window": make_material("M_WindowNight", (0.05, 0.08, 0.2), 0.0, 0.2, emissive=(0.15, 0.25, 0.6)),
    "plant": make_material("M_Plant", (0.05, 0.25, 0.06), 0.0, 0.8),
    "pot": make_material("M_Pot", (0.45, 0.2, 0.1), 0.0, 0.7),
    "velvet": make_material("M_Velvet", (0.12, 0.01, 0.03), 0.0, 0.95),
    "velvet_dark": make_material("M_VelvetDark", (0.05, 0.005, 0.015), 0.0, 1.0),
    "marble": make_material("M_Marble", (0.75, 0.72, 0.68), 0.0, 0.25),
    "show_floor": make_material("M_ShowFloor", (0.03, 0.025, 0.03), 0.3, 0.2),
    "glow_gold": make_material("M_GlowGold", (1.0, 0.7, 0.25), 1.0, 0.3, emissive=(2.0, 1.2, 0.3)),
    "piggy": make_material("M_PiggyPink", (0.95, 0.45, 0.58), 0.0, 0.25),
    "piggy_dark": make_material("M_PiggyPinkDark", (0.8, 0.32, 0.45), 0.0, 0.35),
    "spell_cover": make_material("M_SpellCover", (0.2, 0.04, 0.3), 0.0, 0.5),
    "rune": make_material("M_RuneSoft", (0.6, 0.3, 1.0), 0.0, 0.4, emissive=(0.9, 0.35, 1.8)),
}



def make_coin_face_material():
    """Coin yuzleri: Face (doku) x Tint -> renk, Glow -> parlama. Metalik."""
    path = f"{MAT_DIR}/M_CoinFace"
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    mat = asset_tools.create_asset("M_CoinFace", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    tex = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -700, 0)
    tex.set_editor_property("parameter_name", "Face")
    tex.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
    tint = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -700, 250)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    glow = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -700, 400)
    glow.set_editor_property("parameter_name", "Glow")
    glow.set_editor_property("default_value", 0.0)
    mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -400, 100)
    mel.connect_material_expressions(tex, "RGB", mul, "A")
    mel.connect_material_expressions(tint, "", mul, "B")
    mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
    emi = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, 300)
    mel.connect_material_expressions(mul, "", emi, "A")
    mel.connect_material_expressions(glow, "", emi, "B")
    mel.connect_material_property(emi, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    met = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 450)
    met.set_editor_property("r", 0.85)
    mel.connect_material_property(met, "", unreal.MaterialProperty.MP_METALLIC)
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 550)
    rough.set_editor_property("r", 0.32)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    return mat


make_coin_face_material()

# Menu coin yagmuru ve efektler instanced mesh kullanir: materyallere izin ver (yoksa beyaz gorunur)
for _path in eal.list_assets(MAT_DIR, recursive=False):
    try:
        _m = unreal.load_asset(_path)
        if isinstance(_m, unreal.Material) and not _m.get_editor_property("used_with_instanced_static_meshes"):
            _m.set_editor_property("used_with_instanced_static_meshes", True)
            mel.recompile_material(_m)
            eal.save_loaded_asset(_m)
    except Exception as _e:
        unreal.log_warning(f"Materyal ayari: {_path}: {_e}")

# ---------------------------------------------------------------- yardimcilar
def part(label, mesh, loc, size, mat, rot=None, parent=None, folder="Props"):
    """size: santimetre (x, y, z). Basic Shapes 100 cm ve merkezden pivotlu."""
    a = actors.spawn_actor_from_class(unreal.StaticMeshActor, V(*loc), rot or R())
    smc = a.static_mesh_component
    smc.set_static_mesh(mesh)
    smc.set_material(0, mat)
    smc.set_collision_profile_name("NoCollision")
    a.set_actor_scale3d(V(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    a.set_actor_label(label)
    a.set_folder_path(folder)
    if parent is not None:
        smc.set_mobility(MOVABLE)
        a.attach_to_actor(parent, "", KEEP, KEEP, KEEP, False)
    return a


def point_light(label, loc, intensity, radius, color, folder="Decor", shadows=True):
    l = actors.spawn_actor_from_class(unreal.PointLight, V(*loc), R())
    lc = l.light_component
    lc.set_intensity(intensity)
    lc.set_attenuation_radius(radius)
    lc.set_light_color(unreal.LinearColor(color[0], color[1], color[2], 1.0))
    lc.set_cast_shadows(shadows)
    l.set_actor_label(label)
    l.set_folder_path(folder)
    return l


def interactable(label, action, text, loc, extent):
    a = actors.spawn_actor_from_class(unreal.FortuneInteractable, V(*loc), R())
    a.set_actor_label(label)
    a.set_folder_path("Interactables")
    if action is not None:
        a.set_editor_property("action", action)
    else:
        unreal.log_warning(f"{label}: bu eylem C++'ta yok (projeyi derledin mi?). Esya tiklanmayacak.")
    a.set_editor_property("label", unreal.Text(text))
    a.get_editor_property("click_box").set_box_extent(V(*extent))
    return a


# ---------------------------------------------------------------- 0) Level ve temizlik
level_sub.load_level(MAP_PATH)
for a in actors.get_all_level_actors():
    folder = str(a.get_folder_path())
    if a.get_actor_label() == "Coin" or folder.split("/")[0] in ("Props", "Decor", "Interactables", "Gameplay", "Showcase", "MenuStage"):
        actors.destroy_actor(a)

# ---------------------------------------------------------------- 1) Coin data asset'leri
COINS = [
    # id, isim, nadirlik, agirlik, temel, seviye basina, pasif, pasif deger, pasif yazi, materyal, tanitim
    ("DA_Coin_Bronze", "Bronze Coin", "COMMON", 100, 1.0, 0.5, "NONE", 0.0, "Pasif yok.", "bronze",
     "Her seyin basladigi eski bir bronz para."),
    ("DA_Coin_Silver", "Silver Coin", "UNCOMMON", 50, 2.5, 1.0, "BONUS_HEAD_CHANCE", 0.03, "+%3 yazi sansi.", "silver",
     "Parlak gumus; yaziya biraz daha meyilli."),
    ("DA_Coin_Gold", "Gold Coin", "RARE", 20, 6.0, 2.5, "BONUS_CRIT_CHANCE", 0.05, "+%5 kritik sansi.", "gold",
     "Agir ve sicak. Dustugunde masada cinlar."),
    ("DA_Coin_Clover", "Lucky Clover", "EPIC", 6, 15.0, 6.0, "BONUS_SCROLL_CHANCE", 1.0, "Enchant Scroll dusme sansi x2.", "clover",
     "Dort yaprakli yoncanin damgasi var. Sans pesinden gelir."),
    ("DA_Coin_Phoenix", "Phoenix Coin", "LEGENDARY", 1.5, 40.0, 15.0, "MONEY_MULTIPLIER", 0.5, "Kazanc x1.5.", "phoenix",
     "Kulleriyle yeniden dogan anka kusu. Avucunda yanar."),
]
if not eal.does_directory_exist(COIN_DIR):
    eal.make_directory(COIN_DIR)
for cid, name, rarity, weight, base, per, ptype, pval, ptext, mat, desc in COINS:
    path = f"{COIN_DIR}/{cid}"
    if eal.does_asset_exist(path):
        da = unreal.load_asset(path)
    else:
        f = unreal.DataAssetFactory()
        f.set_editor_property("data_asset_class", unreal.FortuneCoinData)
        da = asset_tools.create_asset(cid, COIN_DIR, unreal.FortuneCoinData, f)
    da.set_editor_property("display_name", unreal.Text(name))
    da.set_editor_property("description", unreal.Text(desc))
    da.set_editor_property("rarity", enum_value(unreal.FortuneRarity, rarity))
    da.set_editor_property("summon_weight", float(weight))
    da.set_editor_property("base_value", float(base))
    da.set_editor_property("value_per_level", float(per))
    da.set_editor_property("passive_type", enum_value(unreal.FortunePassiveType, ptype))
    da.set_editor_property("passive_value", float(pval))
    da.set_editor_property("passive_description", unreal.Text(ptext))
    da.set_editor_property("coin_material", M[mat])
    icon = {"bronze": (0.62, 0.36, 0.18), "silver": (0.85, 0.86, 0.9), "gold": (1.0, 0.72, 0.25),
            "clover": (0.2, 0.75, 0.35), "phoenix": (1.0, 0.35, 0.08)}.get(mat, (0.9, 0.7, 0.3))
    da.set_editor_property("icon_color", unreal.LinearColor(icon[0], icon[1], icon[2], 1.0))
    # Sekil + yazi/tura yuz resimleri (Art/Coins/*.png)
    try:
        coin_key = cid.replace("DA_Coin_", "")
        shape = {"Bronze": "ROUND", "Silver": "OCTAGON", "Gold": "SCALLOPED", "Clover": "DECAGON", "Phoenix": "GEAR"}.get(coin_key, "ROUND")
        sv = enum_value(unreal.FortuneCoinShape, shape)
        if sv is not None:
            da.set_editor_property("shape", sv)
        for side, prop in (("Heads", "heads_texture"), ("Tails", "tails_texture")):
            t = import_texture(f"Art/Coins/T_Coin_{coin_key}_{side}.png", COIN_DIR + "/Textures", f"T_Coin_{coin_key}_{side}")
            if t:
                da.set_editor_property(prop, t)
        rim = {"Bronze": (0.55, 0.3, 0.12), "Silver": (0.7, 0.72, 0.78), "Gold": (0.95, 0.65, 0.15),
               "Clover": (0.75, 0.65, 0.3), "Phoenix": (0.8, 0.3, 0.08)}.get(coin_key, (0.75, 0.5, 0.15))
        da.set_editor_property("rim_color", unreal.LinearColor(rim[0], rim[1], rim[2], 1.0))
    except Exception as e:
        unreal.log_warning(f"Coin gorunumu ayarlanamadi ({cid}): {e}")
    land = {"bronze": "DUST", "silver": "SPARKS", "gold": "GOLD_RING", "clover": "LEAVES", "phoenix": "FLAME"}.get(mat, "DUST")
    try:
        val = enum_value(unreal.FortuneLandFX, land)
        if val is not None:
            da.set_editor_property("land_fx", val)
        else:
            unreal.log_warning(f"LandFX ayarlanamadi: {land} (secenekler: {[a for a in dir(unreal.FortuneLandFX) if a.isupper()]})")
    except Exception as e:
        unreal.log_warning(f"LandFX ayarlanamadi: {e}")
    eal.save_loaded_asset(da)

# ---------------------------------------------------------------- 2) Coin (tiklayinca / Space ile atilir)
flipper = actors.spawn_actor_from_class(unreal.FortuneCoinFlipper, V(-25, 0, TABLE_Z), R())
flipper.set_actor_label("CoinFlipper")
flipper.set_folder_path("Gameplay")
flipper.get_editor_property("coin_mesh").set_material(0, M["bronze"])
interactable("Click_Coin", unreal.FortuneInteraction.FLIP_COIN, "Coin At", (-25, 0, TABLE_Z + 3), (5, 5, 3))

# ---------------------------------------------------------------- 3) Sandik = ENVANTER (sol)
cx, cy = 22.0, -26.0
chest = interactable("Click_Inventory", unreal.FortuneInteraction.INVENTORY, "Envanter", (cx, cy, TABLE_Z + 9), (9, 13, 9.5))
part("Chest_Body", CUBE, (cx, cy, TABLE_Z + 5), (14, 22, 10), M["chest"], parent=chest)
part("Chest_Lid", CYL, (cx, cy, TABLE_Z + 10), (14, 14, 22), M["chest"], rot=R(roll=90), parent=chest)
for i, dy in enumerate((-7.0, 7.0)):
    part(f"Chest_Band_{i}", CUBE, (cx, cy + dy, TABLE_Z + 5), (14.6, 1.2, 10.4), M["gold"], parent=chest)
    part(f"Chest_LidBand_{i}", CYL, (cx, cy + dy, TABLE_Z + 10), (14.6, 14.6, 1.2), M["gold"], rot=R(roll=90), parent=chest)
part("Chest_Lock", CUBE, (cx - 7.4, cy, TABLE_Z + 9), (1.2, 3, 3.5), M["gold"], parent=chest)

# ---------------------------------------------------------------- 4) KUMBARA = COIN COLLECTION (sag)
bx, by = 22.0, 27.0
book = interactable("Click_Collection", unreal.FortuneInteraction.COIN_COLLECTION, "Coin Collection", (bx, by, TABLE_Z + 7), (11, 9, 7.5))
part("Piggy_Body", SPH, (bx, by, TABLE_Z + 7), (17, 13, 12), M["piggy"], parent=book)
part("Piggy_Snout", CYL, (bx - 8.6, by, TABLE_Z + 7.5), (5, 5, 2.2), M["piggy_dark"], rot=R(pitch=90), parent=book)
for i, dy in enumerate((-0.9, 0.9)):
    part(f"Piggy_Nostril_{i}", CYL, (bx - 9.8, by + dy, TABLE_Z + 7.5), (0.9, 0.9, 0.3), M["dark"], rot=R(pitch=90), parent=book)
    part(f"Piggy_Eye_{i}", SPH, (bx - 6.2, by + dy * 2.8, TABLE_Z + 10.3), (1.2, 1.2, 1.2), M["dark"], parent=book)
    part(f"Piggy_Ear_{i}", CONE, (bx - 3.5, by + dy * 4.2, TABLE_Z + 13.2), (3.2, 2.4, 3.2), M["piggy_dark"], rot=R(roll=dy * -20), parent=book)
for i, (lx, ly) in enumerate(((-4.5, -3.5), (-4.5, 3.5), (4.5, -3.5), (4.5, 3.5))):
    part(f"Piggy_Leg_{i}", CYL, (bx + lx, by + ly, TABLE_Z + 1.2), (3, 3, 2.4), M["piggy_dark"], parent=book)
part("Piggy_Tail", SPH, (bx + 8.6, by, TABLE_Z + 8.5), (1.6, 1.6, 1.6), M["piggy_dark"], parent=book)
part("Piggy_Slot", CUBE, (bx + 1, by, TABLE_Z + 12.9), (5, 0.8, 0.6), M["dark"], parent=book)
part("Piggy_SlotCoin", CYL, (bx + 1, by, TABLE_Z + 14.3), (4, 4, 0.5), M["gold"], rot=R(roll=90), parent=book)

# ---------------------------------------------------------------- 5) Gacha HAZINE SANDIGI (arka orta)
# Model ve acilis animasyonu C++ (FortuneGachaChest) icinde; burada sadece yerlestiriliyor.
gacha = actors.spawn_actor_from_class(unreal.FortuneGachaChest, V(50, 0, TABLE_Z + 14), R())
gacha.set_actor_label("GachaChest")
gacha.set_folder_path("Interactables")

# ---------------------------------------------------------------- 6) Ors = YUKSELTMELER (on sol)
ax, ay = -22.0, -32.0
anvil = interactable("Click_Upgrades", unreal.FortuneInteraction.UPGRADES, "Yukseltmeler", (ax, ay, TABLE_Z + 6), (11, 9, 6.5))
part("Anvil_Base", CUBE, (ax, ay, TABLE_Z + 2), (9, 7, 4), M["iron"], parent=anvil)
part("Anvil_Waist", CUBE, (ax, ay, TABLE_Z + 5.5), (5, 4, 3), M["iron"], parent=anvil)
part("Anvil_Top", CUBE, (ax, ay, TABLE_Z + 9), (13, 6, 4), M["iron"], parent=anvil)
part("Anvil_Horn", CONE, (ax + 9, ay, TABLE_Z + 9.5), (4, 4, 6), M["iron"], rot=R(pitch=-90), parent=anvil)
part("Hammer_Handle", CYL, (ax - 2, ay + 8, TABLE_Z + 1), (1.4, 1.4, 14), M["chest"], rot=R(pitch=90, yaw=20), parent=anvil)
part("Hammer_Head", CUBE, (ax - 8.5, ay + 5.6, TABLE_Z + 1.6), (3, 5, 3), M["iron"], rot=R(yaw=20), parent=anvil)


# ---------------------------------------------------------------- 6b) BUYU KITABI = ENCHANT (orta)
s_x, s_y = 14.0, 0.0
spell = interactable("Click_Enchant", enum_value(unreal.FortuneInteraction, "ENCHANT"), "Enchant", (s_x, s_y, TABLE_Z + 5), (11, 13, 6))
part("Spell_Stand", CUBE, (s_x, s_y, TABLE_Z + 1), (12, 16, 2), M["chest"], parent=spell)
part("Spell_CoverL", CUBE, (s_x, s_y - 6.5, TABLE_Z + 3.2), (13, 12, 0.8), M["spell_cover"], rot=R(roll=-10), parent=spell)
part("Spell_CoverR", CUBE, (s_x, s_y + 6.5, TABLE_Z + 3.2), (13, 12, 0.8), M["spell_cover"], rot=R(roll=10), parent=spell)
part("Spell_PagesL", CUBE, (s_x, s_y - 6.2, TABLE_Z + 4.0), (12, 11, 1.2), M["paper"], rot=R(roll=-10), parent=spell)
part("Spell_PagesR", CUBE, (s_x, s_y + 6.2, TABLE_Z + 4.0), (12, 11, 1.2), M["paper"], rot=R(roll=10), parent=spell)
part("Spell_Spine", CYL, (s_x, s_y, TABLE_Z + 3.0), (2.2, 2.2, 13), M["gold"], rot=R(pitch=90), parent=spell)
part("Spell_RuneL", CYL, (s_x, s_y - 6.0, TABLE_Z + 4.75), (6, 6, 0.2), M["rune"], rot=R(roll=-10), parent=spell)
part("Spell_RuneR", CUBE, (s_x, s_y + 6.0, TABLE_Z + 4.75), (7, 5, 0.2), M["rune"], rot=R(roll=10, yaw=45), parent=spell)
part("Spell_Orb", SPH, (s_x, s_y, TABLE_Z + 12), (2.5, 2.5, 2.5), M["rune"], parent=spell)
point_light("Light_Spell", (s_x, s_y, TABLE_Z + 12), 120.0, 50.0, (0.7, 0.35, 1.0), shadows=False)

# ---------------------------------------------------------------- 7) Dekor: para kuleleri, mum, masa lambasi
for s, (sx, sy, n) in enumerate([(-24, 30, 6), (-18, 35, 4), (-28, 36, 3)]):
    for i in range(n):
        part(f"CoinStack_{s}_{i}", CYL, (sx + (i % 2) * 0.3, sy, TABLE_Z + 0.35 + i * 0.7), (4, 4, 0.6), M["gold"])
part("Candle", CYL, (-8, 38, TABLE_Z + 5), (3, 3, 10), M["candle"])
part("Candle_Plate", CYL, (-8, 38, TABLE_Z + 0.3), (7, 7, 0.6), M["gold"])
part("Candle_Flame", SPH, (-8, 38, TABLE_Z + 11), (1.2, 1.2, 2.2), M["flame"], folder="Decor")
point_light("Light_Candle", (-8, 38, TABLE_Z + 13), 250.0, 180.0, (1.0, 0.6, 0.25), shadows=True)

lx, ly = 62.0, -36.0
part("Lamp_Base", CYL, (lx, ly, TABLE_Z + 0.8), (12, 12, 1.6), M["gold"])
part("Lamp_Pole", CYL, (lx, ly, TABLE_Z + 18), (1.5, 1.5, 34), M["gold"])
part("Lamp_Shade", CONE, (lx, ly, TABLE_Z + 38), (20, 20, 14), M["lampshade"])
point_light("Light_TableLamp", (lx, ly, TABLE_Z + 33), 1500.0, 400.0, (1.0, 0.75, 0.45))

# ---------------------------------------------------------------- 8) Oda dekoru
part("Rug_Border", CUBE, (0, 0, 0.4), (280, 200, 0.8), M["rug_border"], folder="Decor")
part("Rug", CUBE, (0, 0, 0.6), (264, 184, 0.8), M["rug"], folder="Decor")
# Ahsap lambri (alt duvar)
part("Wainscot_N", CUBE, (298, 0, 50), (4, 600, 100), M["panel"], folder="Decor")
part("Wainscot_E", CUBE, (0, 298, 50), (600, 4, 100), M["panel"], folder="Decor")
part("Wainscot_W", CUBE, (0, -298, 50), (600, 4, 100), M["panel"], folder="Decor")
part("Wainscot_N_Rail", CUBE, (296, 0, 101), (6, 600, 3), M["gold"], folder="Decor")
# Raf + uzerinde kitaplar ve kavanozlar (karsi duvar)
part("Shelf", CUBE, (288, 0, 165), (22, 180, 3), M["panel"], folder="Decor")
for i, (y, h, m) in enumerate([(-80, 26, "leather_red"), (-73, 22, "leather_green"), (-67, 28, "chest"), (-60, 24, "leather_red")]):
    part(f"ShelfBook_{i}", CUBE, (288, y, 166.5 + h / 2), (16, 5, h), M[m], folder="Decor")
for i, y in enumerate((-20, 5, 30)):
    part(f"Jar_{i}", CYL, (288, y, 176), (12, 12, 18), M["glass"], folder="Decor")
    for j in range(5):
        part(f"Jar_{i}_Coin_{j}", CYL, (288, y, 168 + j * 1.0), (9, 9, 0.8), M["gold"], folder="Decor")
part("Shelf_Trophy", SPH, (288, 70, 176), (14, 14, 14), M["gold"], folder="Decor")
# Tablolar
for i, (y, m) in enumerate(((-190, "canvas_a"), (190, "canvas_b"))):
    part(f"Frame_{i}", CUBE, (297, y, 175), (3, 64, 50), M["gold"], folder="Decor")
    part(f"Painting_{i}", CUBE, (295, y, 175), (2, 56, 42), M[m], folder="Decor")
# Pencere (gece)
part("Window_Frame", CUBE, (40, -299, 175), (130, 3, 100), M["panel"], folder="Decor")
part("Window_Glass", CUBE, (40, -297, 175), (118, 2, 88), M["window"], folder="Decor")
part("Window_BarV", CUBE, (40, -296, 175), (3, 3, 88), M["panel"], folder="Decor")
part("Window_BarH", CUBE, (40, -296, 175), (118, 3, 3), M["panel"], folder="Decor")
moon = actors.spawn_actor_from_class(unreal.SpotLight, V(40, -280, 200), R(pitch=-35, yaw=90))
moon.set_actor_label("Light_Moon")
moon.set_folder_path("Decor")
moon.spot_light_component.set_intensity(3000.0)
moon.spot_light_component.set_light_color(unreal.LinearColor(0.55, 0.65, 1.0, 1.0))
moon.spot_light_component.set_outer_cone_angle(45.0)
# Saksi bitkisi + ayakli lamba
part("Plant_Pot", CYL, (250, -250, 20), (36, 36, 40), M["pot"], folder="Decor")
part("Plant_Leaves", SPH, (250, -250, 70), (70, 70, 80), M["plant"], folder="Decor")
part("FloorLamp_Base", CYL, (250, 250, 1.5), (40, 40, 3), M["gold"], folder="Decor")
part("FloorLamp_Pole", CYL, (250, 250, 80), (4, 4, 160), M["gold"], folder="Decor")
part("FloorLamp_Shade", CONE, (250, 250, 165), (50, 50, 35), M["lampshade"], folder="Decor")
point_light("Light_FloorLamp", (250, 250, 150), 4000.0, 700.0, (1.0, 0.7, 0.4))

# ---------------------------------------------------------------- 9) Isiklar ve post process
for a in actors.get_all_level_actors():
    if a.get_actor_label() == "Light_Ceiling":
        a.light_component.set_intensity(2500.0)
    if a.get_actor_label() == "Light_TableSpot":
        a.spot_light_component.set_intensity(9000.0)
        a.spot_light_component.set_light_color(unreal.LinearColor(1.0, 0.9, 0.75, 1.0))

try:
    ppv = actors.spawn_actor_from_class(unreal.PostProcessVolume, V(0, 0, 150), R())
    ppv.set_actor_label("PostProcess")
    ppv.set_folder_path("Decor")
    ppv.set_editor_property("unbound", True)
    s = ppv.get_editor_property("settings")
    for k, v in (("vignette_intensity", 0.55), ("bloom_intensity", 0.9), ("white_temp", 5800.0)):
        s.set_editor_property("override_" + k, True)
        s.set_editor_property(k, v)
    ppv.set_editor_property("settings", s)
except Exception as e:
    unreal.log_warning("PostProcess ayarlanamadi: " + str(e))

# ---------------------------------------------------------------- 10) Kamera: tum masayi gorsun
target = V(12, 0, TABLE_Z + 4)
cam = None
for a in actors.get_all_level_actors():
    if isinstance(a, unreal.CameraActor) and a.get_actor_label() not in ("CollectionCamera", "MenuCamera"):
        cam = a
        break
if cam is None:
    cam = actors.spawn_actor_from_class(unreal.CameraActor, V(0, 0, 0), R())
    cam.set_actor_label("CoinCamera")
cam_loc = V(-100, 0, 140)
d = target - cam_loc
pitch = math.degrees(math.atan2(d.z, math.hypot(d.x, d.y)))
yaw = math.degrees(math.atan2(d.y, d.x))
cam.set_actor_location_and_rotation(cam_loc, R(pitch=pitch, yaw=yaw), False, False)
cam.camera_component.set_editor_property("field_of_view", 60.0)
cam.set_editor_property("tags", [unreal.Name("FortuneCamera")])


# ---------------------------------------------------------------- 10b) COIN COLLECTION VITRIN ODASI
# Coin Collection acilinca kamera buraya gecer; coin kaidenin ustunde doner.
SX, SY = 0.0, 3000.0
SF = "Showcase"
def sp(label, mesh, loc, size, mat, rot=None):
    return part(label, mesh, (SX + loc[0], SY + loc[1], loc[2]), size, mat, rot=rot, folder=SF)

sp("Show_Floor", CUBE, (0, 0, -5), (520, 520, 10), M["show_floor"])
sp("Show_Ceiling", CUBE, (0, 0, 355), (520, 520, 10), M["velvet_dark"])
sp("Show_WallBack", CUBE, (255, 0, 175), (10, 520, 350), M["velvet_dark"])
sp("Show_WallL", CUBE, (0, -255, 175), (520, 10, 350), M["velvet_dark"])
sp("Show_WallR", CUBE, (0, 255, 175), (520, 10, 350), M["velvet_dark"])
sp("Show_WallFront", CUBE, (-255, 0, 175), (10, 520, 350), M["velvet_dark"])
# Kadife perde (arka duvar, kivrimlar)
for i in range(13):
    y = -180 + i * 30
    sp(f"Curtain_{i}", CYL, (235, y, 160), (26, 26, 320), M["velvet"])
sp("Curtain_Top", CUBE, (228, 0, 322), (20, 420, 22), M["glow_gold"])
# Altin kemer/cerceve
sp("Arch_L", CYL, (215, -150, 150), (16, 16, 300), M["gold"])
sp("Arch_R", CYL, (215, 150, 150), (16, 16, 300), M["gold"])
sp("Arch_Top", CYL, (215, 0, 300), (16, 16, 316), M["gold"], rot=R(roll=90))
# Yuvarlak platform + kaide
sp("Platform", CYL, (0, 0, 4), (180, 180, 8), M["velvet"])
sp("Platform_Ring", CYL, (0, 0, 2), (196, 196, 4), M["gold"])
sp("Pedestal_Base", CYL, (0, 0, 14), (70, 70, 12), M["marble"])
sp("Pedestal_Column", CYL, (0, 0, 55), (44, 44, 72), M["marble"])
sp("Pedestal_Top", CYL, (0, 0, 95), (66, 66, 8), M["marble"])
sp("Pedestal_Trim", CYL, (0, 0, 99.5), (68, 68, 1.5), M["glow_gold"])
# Yan raflar: sergilenen coinler
for side, sy in (("L", -200), ("R", 200)):
    for row, z in enumerate((110, 170, 230)):
        sp(f"Shelf_{side}_{row}", CUBE, (40, sy, z), (220, 30, 4), M["panel"])
        for k in range(5):
            sp(f"ShelfCoin_{side}_{row}_{k}", CYL, (-40 + k * 40, sy, z + 13), (20, 20, 2), M["gold"], rot=R(roll=90))
# Kose sutunlari
for i, (cx, cy) in enumerate(((200, -235), (200, 235), (-200, -235), (-200, 235))):
    sp(f"Pillar_{i}", CYL, (cx, cy, 175), (30, 30, 350), M["marble"])

# Isiklar
def show_spot(label, loc, rot, intensity, color, cone=25.0):
    l = actors.spawn_actor_from_class(unreal.SpotLight, V(SX + loc[0], SY + loc[1], loc[2]), rot)
    l.set_actor_label(label)
    l.set_folder_path(SF)
    c = l.spot_light_component
    c.set_intensity(intensity)
    c.set_light_color(unreal.LinearColor(color[0], color[1], color[2], 1.0))
    c.set_outer_cone_angle(cone)
    c.set_inner_cone_angle(cone * 0.6)
    return l
show_spot("Show_TopSpot", (0, 0, 340), R(pitch=-90), 30000.0, (1.0, 0.85, 0.6), 22.0)
show_spot("Show_RimL", (180, -160, 260), R(pitch=-40, yaw=140), 15000.0, (0.6, 0.3, 1.0), 30.0)
show_spot("Show_RimR", (180, 160, 260), R(pitch=-40, yaw=-140), 15000.0, (1.0, 0.4, 0.2), 30.0)
show_spot("Show_Curtain", (-150, 0, 330), R(pitch=-25), 12000.0, (1.0, 0.3, 0.35), 45.0)
point_light("Show_Fill", (SX - 120, SY, 160), 1200.0, 600.0, (1.0, 0.9, 0.8)).set_folder_path(SF)

# Coin onizleme noktasi + kamera
pp = actors.spawn_actor_from_class(unreal.TargetPoint, V(SX, SY, 135), R())
pp.set_actor_label("CollectionPreviewPoint")
pp.set_folder_path(SF)
pp.set_editor_property("tags", [unreal.Name("CollectionPreviewPoint")])

show_cam = actors.spawn_actor_from_class(unreal.CameraActor, V(SX - 110, SY, 140), R())
show_cam.set_actor_label("CollectionCamera")
show_cam.set_folder_path(SF)
sc_target = V(SX, SY, 127)
sd = sc_target - V(SX - 110, SY, 140)
show_cam.set_actor_rotation(R(pitch=math.degrees(math.atan2(sd.z, math.hypot(sd.x, sd.y))), yaw=0.0), False)
show_cam.camera_component.set_editor_property("field_of_view", 60.0)
show_cam.set_editor_property("tags", [unreal.Name("CollectionCamera")])
show_cam.set_editor_property("auto_activate_for_player", unreal.AutoReceiveInput.DISABLED)


# ---------------------------------------------------------------- 10c) ANA MENU SAHNESI + LOGO
# Logo dokusunu ice aktar (Art/Logo/FFF_Logo.png -> /Game/FlipForFortune/UI/T_FFF_Logo)
try:
    logo_file = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()) + "Art/Logo/FFF_Logo.png"
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", logo_file)
    task.set_editor_property("destination_path", ROOT + "/UI")
    task.set_editor_property("destination_name", "T_FFF_Logo")
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    asset_tools.import_asset_tasks([task])
    tex = unreal.load_asset(ROOT + "/UI/T_FFF_Logo")
    if tex:
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        tex.set_editor_property("never_stream", True)
        eal.save_loaded_asset(tex)
except Exception as e:
    unreal.log_warning("Logo ice aktarilamadi: " + str(e))

MX, MY = 0.0, -3000.0
MF = "MenuStage"
def mp(label, mesh, loc, size, mat, rot=None):
    return part(label, mesh, (MX + loc[0], MY + loc[1], loc[2]), size, mat, rot=rot, folder=MF)

# Koyu kadife fon + altin cerceveli kemerler
mp("Menu_Back", CUBE, (200, 0, 150), (10, 900, 500), M["velvet_dark"])
mp("Menu_Floor", CUBE, (0, 0, -5), (600, 900, 10), M["show_floor"])
for i in range(9):
    mp(f"Menu_Curtain_{i}", CYL, (185, -320 + i * 80, 150), (60, 60, 500), M["velvet"])
mp("Menu_Glow", CUBE, (175, -120, 150), (2, 260, 300), M["glow_gold"])
for side in (-1, 1):
    mp(f"Menu_Pillar_{side}", CYL, (150, side * 330, 150), (40, 40, 400), M["marble"])
# Isiklar: soldaki coin yagmuruna sicak spot, arkadan renkli kenar isiklari
def menu_spot(label, loc, rot, intensity, color, cone):
    l = actors.spawn_actor_from_class(unreal.SpotLight, V(MX + loc[0], MY + loc[1], loc[2]), rot)
    l.set_actor_label(label)
    l.set_folder_path(MF)
    c = l.spot_light_component
    c.set_intensity(intensity)
    c.set_light_color(unreal.LinearColor(color[0], color[1], color[2], 1.0))
    c.set_outer_cone_angle(cone)
    c.set_inner_cone_angle(cone * 0.6)
menu_spot("Menu_KeySpot", (-220, -150, 300), R(pitch=-35, yaw=10), 40000.0, (1.0, 0.85, 0.6), 30.0)
menu_spot("Menu_RimL", (120, -260, 260), R(pitch=-30, yaw=200), 20000.0, (0.7, 0.35, 1.0), 35.0)
menu_spot("Menu_RimR", (120, 120, 260), R(pitch=-30, yaw=160), 15000.0, (1.0, 0.45, 0.15), 35.0)
point_light("Menu_Fill", (MX - 150, MY, 150), 1500.0, 900.0, (1.0, 0.85, 0.7)).set_folder_path(MF)

# Yagan coinler (kameranin solunda)
rain = actors.spawn_actor_from_class(unreal.FortuneCoinRain, V(MX - 90, MY - 75, 150), R())
rain.set_actor_label("MenuCoinRain")
rain.set_folder_path(MF)

# Menu kamerasi
menu_cam = actors.spawn_actor_from_class(unreal.CameraActor, V(MX - 300, MY, 150), R())
menu_cam.set_actor_label("MenuCamera")
menu_cam.set_folder_path(MF)
menu_cam.camera_component.set_editor_property("field_of_view", 60.0)
menu_cam.set_editor_property("tags", [unreal.Name("MenuCamera")])
menu_cam.set_editor_property("auto_activate_for_player", unreal.AutoReceiveInput.DISABLED)

# ---------------------------------------------------------------- 11) Kaydet
level_sub.save_current_level()
unreal.log("FlipForFortune: oda suslendi. Play'e bas: esyalara tikla, Space = coin at.")

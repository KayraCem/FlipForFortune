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
# True: oda + masa yerine kucuk metro istasyonu (Tools/subway_station.py)
SUBWAY = True


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


def import_normal(rel_file, dest_dir, name):
    t = import_texture(rel_file, dest_dir, name)
    if t:
        try:
            t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            t.set_editor_property("srgb", False)
            eal.save_loaded_asset(t)
        except Exception as e:
            unreal.log_warning(f"Normal map ayari: {name}: {e}")
    return t


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


def make_fx_fire_material():
    """Alev parcaciklari: isik ekleyen (additive), kenarlari yumusak, asla siyah gorunmez."""
    path = f"{MAT_DIR}/M_FX_Fire"
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    mat = asset_tools.create_asset("M_FX_Fire", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    E = mel.create_material_expression
    col = E(mat, unreal.MaterialExpressionVectorParameter, -800, 0)
    col.set_editor_property("parameter_name", "Color")
    col.set_editor_property("default_value", unreal.LinearColor(1.0, 0.4, 0.05, 1.0))
    glow = E(mat, unreal.MaterialExpressionScalarParameter, -800, 200)
    glow.set_editor_property("parameter_name", "Glow")
    glow.set_editor_property("default_value", 3.0)
    fres = E(mat, unreal.MaterialExpressionFresnel, -800, 350)
    inv = E(mat, unreal.MaterialExpressionOneMinus, -600, 350)
    mel.connect_material_expressions(fres, "", inv, "")
    soft = E(mat, unreal.MaterialExpressionPower, -450, 350)
    soft.set_editor_property("const_exponent", 2.0)
    mel.connect_material_expressions(inv, "", soft, "Base")
    m1 = E(mat, unreal.MaterialExpressionMultiply, -450, 100)
    mel.connect_material_expressions(col, "", m1, "A")
    mel.connect_material_expressions(glow, "", m1, "B")
    m2 = E(mat, unreal.MaterialExpressionMultiply, -250, 200)
    mel.connect_material_expressions(m1, "", m2, "A")
    mel.connect_material_expressions(soft, "", m2, "B")
    mel.connect_material_property(m2, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    return mat


try:
    make_fx_fire_material()
except Exception as _e:
    unreal.log_warning(f"M_FX_Fire olusturulamadi: {_e}")


# ---- Efekt materyalleri: seffaf (alpha) karisim -> beyaz fayansin onunde de gorunur (eskiden additive, aydinlikta kayboluyordu)
def _rebuild_mat(name):
    path = f"{MAT_DIR}/{name}"
    if eal.does_asset_exist(path):
        m = unreal.load_asset(path)
        mel.delete_all_material_expressions(m)
        return m
    return asset_tools.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())


def _fx_mats():
    E = mel.create_material_expression
    MP = unreal.MaterialProperty
    for name, soft_edges, opac in (("M_FX_Glow", False, 0.92), ("M_FX_Fire", True, 0.85)):
        m = _rebuild_mat(name)
        m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        m.set_editor_property("used_with_instanced_static_meshes", True)
        col = E(m, unreal.MaterialExpressionVectorParameter, -900, 0)
        col.set_editor_property("parameter_name", "Color")
        col.set_editor_property("default_value", unreal.LinearColor(1.0, 0.6, 0.2, 1.0))
        glow = E(m, unreal.MaterialExpressionScalarParameter, -900, 200)
        glow.set_editor_property("parameter_name", "Glow")
        glow.set_editor_property("default_value", 4.0)
        em = E(m, unreal.MaterialExpressionMultiply, -600, 100)
        mel.connect_material_expressions(col, "", em, "A")
        mel.connect_material_expressions(glow, "", em, "B")
        op = E(m, unreal.MaterialExpressionScalarParameter, -900, 400)
        op.set_editor_property("parameter_name", "Opacity")
        op.set_editor_property("default_value", opac)
        if soft_edges:
            fres = E(m, unreal.MaterialExpressionFresnel, -900, 550)
            inv = E(m, unreal.MaterialExpressionOneMinus, -700, 550)
            mel.connect_material_expressions(fres, "", inv, "")
            soft = E(m, unreal.MaterialExpressionPower, -550, 550)
            soft.set_editor_property("const_exponent", 1.6)
            mel.connect_material_expressions(inv, "", soft, "Base")
            em2 = E(m, unreal.MaterialExpressionMultiply, -400, 100)
            mel.connect_material_expressions(em, "", em2, "A")
            mel.connect_material_expressions(soft, "", em2, "B")
            o2 = E(m, unreal.MaterialExpressionMultiply, -400, 450)
            mel.connect_material_expressions(op, "", o2, "A")
            mel.connect_material_expressions(soft, "", o2, "B")
            mel.connect_material_property(em2, "", MP.MP_EMISSIVE_COLOR)
            mel.connect_material_property(o2, "", MP.MP_OPACITY)
        else:
            mel.connect_material_property(em, "", MP.MP_EMISSIVE_COLOR)
            mel.connect_material_property(op, "", MP.MP_OPACITY)
        mel.recompile_material(m)
        eal.save_loaded_asset(m)

    # ---- UI coin ikonu: stüdyoda cekilen 3D goruntu (render target), seffaf arka plan
    m = _rebuild_mat("M_UI_CoinIcon")
    m.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    tex = E(m, unreal.MaterialExpressionTextureSampleParameter2D, -900, 0)
    tex.set_editor_property("parameter_name", "Icon")
    dflt = unreal.load_asset("/Engine/EngineResources/DefaultTexture")
    if dflt:
        tex.set_editor_property("texture", dflt)
    ex = E(m, unreal.MaterialExpressionScalarParameter, -900, 250)
    ex.set_editor_property("parameter_name", "Exposure")
    ex.set_editor_property("default_value", 1.0)
    inv = E(m, unreal.MaterialExpressionScalarParameter, -900, 350)
    inv.set_editor_property("parameter_name", "Invert")
    inv.set_editor_property("default_value", 1.0)
    tint = E(m, unreal.MaterialExpressionVectorParameter, -900, 450)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    mul = E(m, unreal.MaterialExpressionMultiply, -650, 0)
    mel.connect_material_expressions(tex, "RGB", mul, "A")
    mel.connect_material_expressions(ex, "", mul, "B")
    sat = E(m, unreal.MaterialExpressionSaturate, -500, 0)
    mel.connect_material_expressions(mul, "", sat, "")
    mt = E(m, unreal.MaterialExpressionMultiply, -350, 0)
    mel.connect_material_expressions(sat, "", mt, "A")
    mel.connect_material_expressions(tint, "", mt, "B")
    mel.connect_material_property(mt, "", MP.MP_EMISSIVE_COLOR)
    one = E(m, unreal.MaterialExpressionOneMinus, -650, 250)
    mel.connect_material_expressions(tex, "A", one, "")
    lp = E(m, unreal.MaterialExpressionLinearInterpolate, -450, 250)
    mel.connect_material_expressions(tex, "A", lp, "A")
    mel.connect_material_expressions(one, "", lp, "B")
    mel.connect_material_expressions(inv, "", lp, "Alpha")
    mel.connect_material_property(lp, "", MP.MP_OPACITY)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)


def _plain_mat():
    m = _rebuild_mat("M_AuraPlain")
    E = mel.create_material_expression
    col = E(m, unreal.MaterialExpressionVectorParameter, -500, 0)
    col.set_editor_property("parameter_name", "Color")
    col.set_editor_property("default_value", unreal.LinearColor(0.1, 0.4, 0.1, 1))
    mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    r = E(m, unreal.MaterialExpressionConstant, -500, 300)
    r.set_editor_property("r", 0.85)
    mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    sp = E(m, unreal.MaterialExpressionConstant, -500, 400)
    sp.set_editor_property("r", 0.2)
    mel.connect_material_property(sp, "", unreal.MaterialProperty.MP_SPECULAR)
    m.set_editor_property("used_with_instanced_static_meshes", True)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)


try:
    _plain_mat()
except Exception as _e:
    unreal.log_warning(f"M_AuraPlain: {_e}")

try:
    _fx_mats()
except Exception as _e:
    unreal.log_warning(f"Efekt/ikon materyalleri olusturulamadi: {_e}")

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



def make_coin_face_material2():
    """Kabartmali coin materyali: Face (renk) + FaceNormal (normal map) + Tint + Glow."""
    path = f"{MAT_DIR}/M_CoinFace2"
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    mat = asset_tools.create_asset("M_CoinFace2", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    tex = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -800, 0)
    tex.set_editor_property("parameter_name", "Face")
    tex.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
    nrm = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -800, 600)
    nrm.set_editor_property("parameter_name", "FaceNormal")
    nrm.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    nrm.set_editor_property("texture", unreal.load_asset("/Engine/EngineMaterials/FlatNormal"))
    mel.connect_material_property(nrm, "RGB", unreal.MaterialProperty.MP_NORMAL)
    tint = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -800, 250)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    glow = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -800, 420)
    glow.set_editor_property("parameter_name", "Glow")
    glow.set_editor_property("default_value", 0.0)
    mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -450, 100)
    mel.connect_material_expressions(tex, "RGB", mul, "A")
    mel.connect_material_expressions(tint, "", mul, "B")
    mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
    emi = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -250, 300)
    mel.connect_material_expressions(mul, "", emi, "A")
    mel.connect_material_expressions(glow, "", emi, "B")
    mel.connect_material_property(emi, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    met = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -450, 450)
    met.set_editor_property("r", 0.9)
    mel.connect_material_property(met, "", unreal.MaterialProperty.MP_METALLIC)
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -450, 520)
    rough.set_editor_property("r", 0.28)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    return mat


def make_coin_face_material3():
    """Neon coin materyali: M_CoinFace2 + FaceGlow (isik saçan doku) x GlowColor x GlowStrength, nabiz gibi atar."""
    path = f"{MAT_DIR}/M_CoinFace3"
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    mat = asset_tools.create_asset("M_CoinFace3", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    E = mel.create_material_expression
    tex = E(mat, unreal.MaterialExpressionTextureSampleParameter2D, -900, 0)
    tex.set_editor_property("parameter_name", "Face")
    tex.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
    nrm = E(mat, unreal.MaterialExpressionTextureSampleParameter2D, -900, 700)
    nrm.set_editor_property("parameter_name", "FaceNormal")
    nrm.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    nrm.set_editor_property("texture", unreal.load_asset("/Engine/EngineMaterials/FlatNormal"))
    mel.connect_material_property(nrm, "RGB", unreal.MaterialProperty.MP_NORMAL)
    tint = E(mat, unreal.MaterialExpressionVectorParameter, -900, 250)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    glow = E(mat, unreal.MaterialExpressionScalarParameter, -900, 400)
    glow.set_editor_property("parameter_name", "Glow")
    glow.set_editor_property("default_value", 0.0)
    mul = E(mat, unreal.MaterialExpressionMultiply, -550, 100)
    mel.connect_material_expressions(tex, "RGB", mul, "A")
    mel.connect_material_expressions(tint, "", mul, "B")
    mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
    emi = E(mat, unreal.MaterialExpressionMultiply, -350, 300)
    mel.connect_material_expressions(mul, "", emi, "A")
    mel.connect_material_expressions(glow, "", emi, "B")
    # Neon kismi
    gtex = E(mat, unreal.MaterialExpressionTextureSampleParameter2D, -900, 950)
    gtex.set_editor_property("parameter_name", "FaceGlow")
    gtex.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/Black"))
    gcol = E(mat, unreal.MaterialExpressionVectorParameter, -900, 1200)
    gcol.set_editor_property("parameter_name", "GlowColor")
    gcol.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    gstr = E(mat, unreal.MaterialExpressionScalarParameter, -900, 1350)
    gstr.set_editor_property("parameter_name", "GlowStrength")
    gstr.set_editor_property("default_value", 0.0)
    tim = E(mat, unreal.MaterialExpressionTime, -900, 1450)
    tsp = E(mat, unreal.MaterialExpressionMultiply, -750, 1450)
    tsp.set_editor_property("const_b", 0.7)
    mel.connect_material_expressions(tim, "", tsp, "A")
    sin = E(mat, unreal.MaterialExpressionSine, -600, 1450)
    mel.connect_material_expressions(tsp, "", sin, "")
    amp = E(mat, unreal.MaterialExpressionMultiply, -450, 1450)
    amp.set_editor_property("const_b", 0.25)
    mel.connect_material_expressions(sin, "", amp, "A")
    pulse = E(mat, unreal.MaterialExpressionAdd, -300, 1450)
    pulse.set_editor_property("const_b", 0.75)
    mel.connect_material_expressions(amp, "", pulse, "A")
    g1 = E(mat, unreal.MaterialExpressionMultiply, -600, 1050)
    mel.connect_material_expressions(gtex, "RGB", g1, "A")
    mel.connect_material_expressions(gcol, "", g1, "B")
    g2 = E(mat, unreal.MaterialExpressionMultiply, -450, 1150)
    mel.connect_material_expressions(g1, "", g2, "A")
    mel.connect_material_expressions(gstr, "", g2, "B")
    g3 = E(mat, unreal.MaterialExpressionMultiply, -300, 1150)
    mel.connect_material_expressions(g2, "", g3, "A")
    mel.connect_material_expressions(pulse, "", g3, "B")
    tot = E(mat, unreal.MaterialExpressionAdd, -150, 500)
    mel.connect_material_expressions(emi, "", tot, "A")
    mel.connect_material_expressions(g3, "", tot, "B")
    mel.connect_material_property(tot, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    met = E(mat, unreal.MaterialExpressionConstant, -550, 550)
    met.set_editor_property("r", 0.9)
    mel.connect_material_property(met, "", unreal.MaterialProperty.MP_METALLIC)
    rough = E(mat, unreal.MaterialExpressionConstant, -550, 620)
    rough.set_editor_property("r", 0.28)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    return mat


def make_vertex_color_material():
    """Karakter modelleri: renk tepe noktasi renginden (vertex color)."""
    path = f"{MAT_DIR}/M_VertexColor"
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    mat = asset_tools.create_asset("M_VertexColor", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    vc = mel.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -500, 0)
    mel.connect_material_property(vc, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -500, 250)
    rough.set_editor_property("r", 0.78)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -500, 350)
    spec.set_editor_property("r", 0.35)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    return mat


def make_aura_plain():
    """Yaprak, diken, gul icin metal olmayan duz materyal (Color parametresi)."""
    path = f"{MAT_DIR}/M_AuraPlain"
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    mat = asset_tools.create_asset("M_AuraPlain", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    col = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -500, 0)
    col.set_editor_property("parameter_name", "Color")
    col.set_editor_property("default_value", unreal.LinearColor(0.1, 0.4, 0.1, 1))
    mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -500, 300)
    rough.set_editor_property("r", 0.45)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -500, 400)
    spec.set_editor_property("r", 0.6)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    return mat


def make_aura_metal():
    """Coin zincirleri / kilitleri icin metal: Color parametresi."""
    path = f"{MAT_DIR}/M_AuraMetal"
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    mat = asset_tools.create_asset("M_AuraMetal", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    col = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -500, 0)
    col.set_editor_property("parameter_name", "Color")
    col.set_editor_property("default_value", unreal.LinearColor(0.6, 0.6, 0.65, 1))
    mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    met = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -500, 200)
    met.set_editor_property("r", 1.0)
    mel.connect_material_property(met, "", unreal.MaterialProperty.MP_METALLIC)
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -500, 300)
    rough.set_editor_property("r", 0.3)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    return mat


make_coin_face_material()
make_coin_face_material2()
try:
    make_coin_face_material3()
except Exception as _e:
    unreal.log_warning(f"M_CoinFace3 olusturulamadi: {_e}")
try:
    make_vertex_color_material()
except Exception as _e:
    unreal.log_warning(f"M_VertexColor olusturulamadi: {_e}")
try:
    make_aura_plain()
except Exception as _e:
    unreal.log_warning(f"M_AuraPlain olusturulamadi: {_e}")
try:
    make_aura_metal()
except Exception as _e:
    unreal.log_warning(f"M_AuraMetal olusturulamadi: {_e}")

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
    if a.get_actor_label() == "Coin" or folder.split("/")[0] in ("Props", "Decor", "Interactables", "Gameplay", "Showcase", "MenuStage", "Subway"):
        actors.destroy_actor(a)
    elif SUBWAY and (folder.split("/")[0] == "Room" or a.get_actor_label() in ("Light_Ceiling", "Light_TableSpot")):
        actors.destroy_actor(a)   # eski oda + masa

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
    # ---- yeni coinler
    ("DA_Coin_Copper", "Copper Coin", "COMMON", 90, 1.5, 0.6, "FLAT_MONEY_BONUS", 0.5, "Her atışta +0.5 para.", "bronze",
     "Cebin dibinde unutulmuş beşlik. Az ama düzenli kazandırır."),
    ("DA_Coin_Star", "Star Coin", "UNCOMMON", 45, 3.0, 1.2, "BONUS_CRIT_CHANCE", 0.03, "+%3 kritik şansı.", "silver",
     "Gece gökyüzünden düşmüş bir yıldız parçası."),
    ("DA_Coin_Ghost", "Ghost Coin", "RARE", 18, 8.0, 3.0, "TAILS_FULL_VALUE", 0.0, "Tura da tam değer verir.", "silver",
     "Bazen elinin içinden geçer gibi olur. Turadan bile korkmaz."),
    ("DA_Coin_Gear", "Gear Coin", "RARE", 16, 10.0, 3.5, "BONUS_HEAD_CHANCE", 0.04, "+%4 yazı şansı.", "silver",
     "Saatçinin çekmecesinden çıkmış. Dişlileri hep yazıya döner."),
    ("DA_Coin_Pumpkin", "Pumpkin Coin", "EPIC", 5, 20.0, 8.0, "BONUS_XP", 1.0, "Atış başına +1 XP.", "phoenix",
     "Cadılar bayramından kalma. Hızla büyür, hızla güçlenir."),
    ("DA_Coin_Calendar", "Calendar Coin", "MYTHIC", 0.4, 120.0, 40.0, "MONEY_MULTIPLIER", 1.0, "Kazanç x2.", "gold",
     "Üstündeki kadran yılın her gününü sayar. Zaman paradır."),
    ("DA_Coin_Angel", "Angel Coin", "DIVINE", 0.1, 300.0, 100.0, "BONUS_CRIT_MULTIPLY", 1.0, "Kritik çarpanı +1.", "gold",
     "Bulutların üstünde basılmış. Her kritik vuruşu kutsar."),
    ("DA_Coin_Galaxy", "Galaxy Coin", "CELESTIAL", 0.03, 800.0, 250.0, "BONUS_SCROLL_CHANCE", 3.0, "Enchant Scroll şansı x4.", "clover",
     "İçinde dönen minik bir galaksi var. Şans yıldızların arasında."),
    ("DA_Coin_Seaweed", "Seaweed Coin", "RARE", 15, 9.0, 3.2, "FLAT_MONEY_BONUS", 3.0, "Her atışta +3 para.", "clover",
     "Okyanusun dibinden çıkmış, hâlâ dalgalarla sallanıyor."),
    ("DA_Coin_Zone", "Zone Coin", "EPIC", 5, 22.0, 8.5, "BONUS_CRIT_CHANCE", 0.08, "+%8 kritik şansı.", "clover",
     "Tehlike bölgesi. Yaklaşan her atış kritik olabilir."),
    ("DA_Coin_BlackRose", "Black Rose Coin", "LEGENDARY", 1.3, 45.0, 16.0, "BONUS_CRIT_MULTIPLY", 0.75, "Kritik çarpanı +0.75.", "phoenix",
     "Dikenleri arasında kara bir gül. Güzel ama acıtır."),
    ("DA_Coin_Lock", "Lock Coin", "LEGENDARY", 1.2, 42.0, 15.0, "MONEY_MULTIPLIER", 0.6, "Kazanç x1.6.", "gold",
     "Zincirlerle kilitlenmiş bir hazine. Anahtarı sende."),
    ("DA_Coin_Skull", "Skull Coin", "SECRET", 0.005, 2500.0, 800.0, "MONEY_MULTIPLIER", 3.0, "Kazanç x4.", "phoenix",
     "Kimse nereden geldiğini bilmiyor. Sahibine servet, diğerlerine korku."),
]

# Yeni coinlerin gorunusu: sekil, kenar rengi, seritteki renk, inis efekti
STYLE = {
    # coin: (sekil, kenar rengi, ikon rengi, inis efekti)  -- her coin kendine ozgu sekil
    "Bronze":   ("ROUND",     (0.55, 0.3, 0.12),  (0.85, 0.55, 0.3), "DUST"),
    "Copper":   ("HEXAGON",   (0.7, 0.35, 0.18),  (0.8, 0.42, 0.22), "DUST"),
    "Silver":   ("OCTAGON",   (0.7, 0.72, 0.78),  (0.85, 0.86, 0.9), "SPARKS"),
    "Star":     ("STAR",      (0.85, 0.6, 0.15),  (1.0, 0.8, 0.3), "SPARKS"),
    "Gold":     ("DODECAGON", (0.9, 0.65, 0.18),  (1.0, 0.75, 0.3), "GOLD_RING"),
    "Ghost":    ("GHOST",     (0.7, 0.82, 0.95),  (0.8, 0.9, 1.0), "SPARKS"),
    "Gear":     ("GEAR_RING", (0.45, 0.42, 0.25), (0.55, 0.52, 0.32), "SPARKS"),
    "Seaweed":  ("ROUND",     (0.12, 0.45, 0.42), (0.25, 0.9, 0.75), "LEAVES"),
    "Clover":   ("CLOVER",    (0.15, 0.45, 0.2),  (0.2, 0.75, 0.35), "LEAVES"),
    "Pumpkin":  ("PUMPKIN",   (0.7, 0.32, 0.06),  (1.0, 0.55, 0.12), "LEAVES"),
    "Zone":     ("HEXAGON",   (0.25, 0.28, 0.12), (0.78, 1.0, 0.22), "SPARKS"),
    "Phoenix":  ("GEAR",      (0.3, 0.1, 0.05),   (1.0, 0.35, 0.08), "FLAME"),
    "BlackRose":("SCALLOPED", (0.55, 0.38, 0.12), (0.8, 0.1, 0.18), "LEAVES"),
    "Lock":     ("ROUND",     (0.75, 0.55, 0.18), (1.0, 0.8, 0.25), "GOLD_RING"),
    "Calendar": ("ROUND",     (0.75, 0.52, 0.12), (0.95, 0.72, 0.3), "GOLD_RING"),
    "Angel":    ("SCALLOPED", (0.95, 0.85, 0.55), (1.0, 0.97, 0.85), "GOLD_RING"),
    "Galaxy":   ("DECAGON",   (0.25, 0.12, 0.5),  (0.45, 0.3, 0.9), "SPARKS"),
    "Skull":    ("SKULL",     (0.55, 0.48, 0.38),   (0.3, 0.9, 0.35), "FLAME"),
}
# Isik saçan kisimlar + coine ozel 3D efekt
GLOW = {
    # coin: (parlama gucu, aura, aura rengi)
    "Bronze":   (3.5, "NONE",       (1.0, 0.6, 0.3)),
    "Copper":   (3.5, "BOLTS",      (1.0, 0.48, 0.25)),
    "Silver":   (3.5, "COMPASS",    (0.8, 0.88, 1.0)),
    "Star":     (4.0, "SPARKLE",    (1.0, 0.85, 0.4)),
    "Gold":     (3.5, "SPARKLE",    (1.0, 0.78, 0.25)),
    "Ghost":    (5.0, "SPARKLE",    (0.3, 0.95, 1.0)),
    "Gear":     (3.5, "NONE",       (1.0, 0.7, 0.2)),
    "Seaweed":  (4.0, "KELP",       (0.4, 1.0, 0.85)),
    "Clover":   (3.5, "SPARKLE",    (0.45, 1.0, 0.5)),
    "Pumpkin":  (6.0, "STEM",       (1.0, 0.55, 0.12)),
    "Zone":     (5.0, "ZONE",       (0.78, 1.0, 0.22)),
    "Phoenix":  (5.0, "FIRE",       (1.0, 0.32, 0.05)),
    "BlackRose":(5.0, "ROSE_BLOOM", (1.0, 0.12, 0.2)),
    "Lock":     (4.5, "VAULT",      (1.0, 0.8, 0.25)),
    "Calendar": (4.0, "PAGES",      (1.0, 0.8, 0.35)),
    "Angel":    (4.0, "WINGS",      (1.0, 0.9, 0.5)),
    "Galaxy":   (5.0, "ORBIT",      (0.55, 0.65, 1.0)),
    "Skull":    (6.0, "EYE_FLAMES", (0.3, 1.0, 0.35)),
}
if not eal.does_directory_exist(COIN_DIR):
    eal.make_directory(COIN_DIR)
import_normal("Art/Coins/T_Coin_RimNormal.png", COIN_DIR + "/Textures", "T_Coin_RimNormal")
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
    ck = cid.replace("DA_Coin_", "")
    icon = STYLE[ck][2] if ck in STYLE else {"bronze": (0.62, 0.36, 0.18), "silver": (0.85, 0.86, 0.9), "gold": (1.0, 0.72, 0.25),
            "clover": (0.2, 0.75, 0.35), "phoenix": (1.0, 0.35, 0.08)}.get(mat, (0.9, 0.7, 0.3))
    da.set_editor_property("icon_color", unreal.LinearColor(icon[0], icon[1], icon[2], 1.0))
    # Sekil + yazi/tura yuz resimleri (Art/Coins/*.png)
    try:
        coin_key = cid.replace("DA_Coin_", "")
        shape = {"Bronze": "ROUND", "Silver": "OCTAGON", "Gold": "SCALLOPED", "Clover": "DECAGON", "Phoenix": "GEAR"}.get(coin_key, "ROUND")
        if coin_key in STYLE:
            shape = STYLE[coin_key][0]
        sv = enum_value(unreal.FortuneCoinShape, shape)
        if sv is not None:
            da.set_editor_property("shape", sv)
        for side, prop in (("Heads", "heads_texture"), ("Tails", "tails_texture")):
            t = import_texture(f"Art/Coins/T_Coin_{coin_key}_{side}.png", COIN_DIR + "/Textures", f"T_Coin_{coin_key}_{side}")
            if t:
                da.set_editor_property(prop, t)
            n = import_normal(f"Art/Coins/T_Coin_{coin_key}_{side}_N.png", COIN_DIR + "/Textures", f"T_Coin_{coin_key}_{side}_N")
            if n:
                da.set_editor_property("heads_normal" if side == "Heads" else "tails_normal", n)
        # Neon parlama + tema efekti
        gl = GLOW.get(coin_key)
        if gl:
            strength, aura, aura_col = gl
            for side, prop in (("Heads", "heads_glow"), ("Tails", "tails_glow")):
                g = import_texture(f"Art/Coins/T_Coin_{coin_key}_{side}_G.png", COIN_DIR + "/Textures", f"T_Coin_{coin_key}_{side}_G")
                if g:
                    da.set_editor_property(prop, g)
            da.set_editor_property("glow_color", unreal.LinearColor(1, 1, 1, 1))
            da.set_editor_property("glow_strength", float(strength))
            av = enum_value(unreal.FortuneCoinAura, aura)
            if av is not None:
                da.set_editor_property("aura", av)
            da.set_editor_property("aura_color", unreal.LinearColor(aura_col[0], aura_col[1], aura_col[2], 1.0))
        rim = {"Bronze": (0.55, 0.3, 0.12), "Silver": (0.7, 0.72, 0.78), "Gold": (0.95, 0.65, 0.15),
               "Clover": (0.75, 0.65, 0.3), "Phoenix": (0.8, 0.3, 0.08)}.get(coin_key, (0.75, 0.5, 0.15))
        if coin_key in STYLE:
            rim = STYLE[coin_key][1]
        da.set_editor_property("rim_color", unreal.LinearColor(rim[0], rim[1], rim[2], 1.0))
    except Exception as e:
        unreal.log_warning(f"Coin gorunumu ayarlanamadi ({cid}): {e}")
    land = {"bronze": "DUST", "silver": "SPARKS", "gold": "GOLD_RING", "clover": "LEAVES", "phoenix": "FLAME"}.get(mat, "DUST")
    if ck in STYLE:
        land = STYLE[ck][3]
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
if not SUBWAY:   # metroda: bos yere sol tik / Space = coin at
    interactable("Click_Coin", unreal.FortuneInteraction.FLIP_COIN, "Coin At", (-25, 0, TABLE_Z + 3), (5, 5, 3))

# ANA KARAKTER: masanin karsisinda durur, coini avucundan atar ve avucuna yakalar
USE_HERO = True
try:
    if USE_HERO:
        # Birinci sahis: karakter masanin onunde (kameranin oldugu yerde), masaya bakar
        hero = actors.spawn_actor_from_class(unreal.FortuneHeroActor, V(-95, 0, 0), R())
        hero.set_actor_label("Hero")
        hero.set_folder_path("Gameplay")
        hero.set_actor_scale3d(V(0.92, 0.92, 0.92))
        flipper.set_editor_property("hero", hero)
        if not SUBWAY:
            interactable("Click_HeroCoin", unreal.FortuneInteraction.FLIP_COIN, "Coin At", (-52, 8, 106), (10, 10, 8))
except Exception as e:
    unreal.log_warning("Ana karakter eklenemedi (projeyi derledin mi?): " + str(e))
    USE_HERO = False

# El: coin'i fiskeyle atar (masanin on-sagi, kameraya yakin) - karakter yoksa
try:
    if USE_HERO:
        raise RuntimeError("karakter var, eski el eklenmiyor")
    hand = actors.spawn_actor_from_class(unreal.FortuneHandActor, V(-50, 15, TABLE_Z + 7.5), R(yaw=-18))
    hand.set_actor_label("Hand")
    hand.set_folder_path("Gameplay")
    hand.set_actor_scale3d(V(1.5, 1.5, 1.5))
    flipper.set_editor_property("hand", hand)
    # Projede mannequin kolu varsa (Add > Feature or Content Pack > First Person) onu kullan
    try:
        cands = []
        for path in eal.list_assets("/Game", recursive=True, include_folder=False):
            base = path.split("/")[-1].split(".")[0]
            low = base.lower()
            if low.startswith(("skm_", "sk_")) and any(k in low for k in ("arms", "manny", "quinn", "mannequin")):
                score = 0 if "arms" in low else (1 if "manny" in low else 2)
                cands.append((score, path))
        cands.sort()
        for _, path in cands:
            m = unreal.load_asset(path)
            if isinstance(m, unreal.SkeletalMesh):
                hand.set_editor_property("hand_mesh", m)
                unreal.log(f"El modeli: {path}")
                break
        else:
            unreal.log_warning("Mannequin kolu bulunamadi: kodla kurulan el kullaniliyor (First Person paketini ekle).")
    except Exception as e:
        unreal.log_warning(f"El modeli ayarlanamadi: {e}")
    # Elin uzerindeki coine tiklayinca da atilsin
    interactable("Click_HandCoin", unreal.FortuneInteraction.FLIP_COIN, "Coin At", (-41, 10, TABLE_Z + 11), (8, 8, 6))
except Exception as e:
    unreal.log_warning("El eklenemedi (projeyi derledin mi?): " + str(e))

if not SUBWAY:   # masa esyalari (metroda HUD ikonlari var)
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
gacha = actors.spawn_actor_from_class(unreal.FortuneGachaChest, V(50, 0, TABLE_Z + 14), R())   # karakterin onunu kapatmasin
gacha.set_actor_label("GachaChest")
gacha.set_folder_path("Interactables")

if not SUBWAY:   # masa + oda dekoru
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

# ---------------------------------------------------------------- 8b) METRO ISTASYONU
if SUBWAY:
    _sub = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()) + "Tools/subway_station.py"
    exec(open(_sub, encoding="utf-8").read(), globals())

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
    if isinstance(a, unreal.CameraActor) and a.get_actor_label() not in ("CollectionCamera", "MenuCamera", "SummonCamera"):
        cam = a
        break
if cam is None:
    cam = actors.spawn_actor_from_class(unreal.CameraActor, V(0, 0, 0), R())
    cam.set_actor_label("CoinCamera")
cam_loc = V(-100, 0, 140)
if USE_HERO:
    # Birinci sahis: kamera karakterin goz hizasinda, masaya bakar; el ve coin ekranin altinda
    cam_loc = V(-95 + 10.1, 0, 151.8)
    target = V(20, 0, 80)
d = target - cam_loc
pitch = math.degrees(math.atan2(d.z, math.hypot(d.x, d.y)))
yaw = math.degrees(math.atan2(d.y, d.x))
if SUBWAY:
    pitch, yaw = -26.0, 0.0   # peronun ilerisine bakar; el ve coin ekranin altinda
cam.set_actor_location_and_rotation(cam_loc, R(pitch=pitch, yaw=yaw), False, False)
cam.camera_component.set_editor_property("field_of_view", (90.0 if SUBWAY else 80.0) if USE_HERO else 60.0)
cam.set_editor_property("tags", [unreal.Name("FortuneCamera")])
if USE_HERO:
    try:
        cam.attach_to_actor(hero, "", KEEP, KEEP, KEEP, False)   # karakter yurudugunde kamera da gelir
    except Exception as e:
        unreal.log_warning(f"Kamera karaktere baglanamadi: {e}")


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

# HUD sol menu ikonlari (Art/UI/T_Icon_*.png)
for _icon in ("T_Icon_Collection", "T_Icon_Inventory"):
    try:
        t = import_texture(f"Art/UI/{_icon}.png", ROOT + "/UI", _icon)
        if t:
            t.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
            t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
            t.set_editor_property("never_stream", True)
            eal.save_loaded_asset(t)
    except Exception as e:
        unreal.log_warning(f"Ikon ice aktarilamadi {_icon}: {e}")

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

# Flip for Fortune - Coin odasi kurulum betigi
# Yeni bir level olusturur: oda + masa + masanin ustunde coin + isiklar + kamera.
import math
import unreal

ROOT = "/Game/FlipForFortune"
MAP_PATH = ROOT + "/Maps/CoinRoom"
MAT_DIR = ROOT + "/Materials"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

CUBE = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
CYL = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")


def make_material(name, rgb, metallic, roughness):
    path = f"{MAT_DIR}/{name}"
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    mat = asset_tools.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    col = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 0)
    col.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    met = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 200)
    met.set_editor_property("r", metallic)
    mel.connect_material_property(met, "", unreal.MaterialProperty.MP_METALLIC)
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 300)
    rough.set_editor_property("r", roughness)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    return mat


def box(label, loc, size_cm, mat):
    """size_cm: (x, y, z) santimetre. Basic Cube 100x100x100 ve merkezden pivotlu."""
    a = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc), unreal.Rotator(0, 0, 0))
    a.static_mesh_component.set_static_mesh(CUBE)
    a.static_mesh_component.set_material(0, mat)
    a.set_actor_scale3d(unreal.Vector(size_cm[0] / 100, size_cm[1] / 100, size_cm[2] / 100))
    a.set_actor_label(label)
    a.set_folder_path("Room")
    return a


# 1) Yeni, bos level (eski arazi haritasi kaydedilmeden kapanir)
level_sub.new_level(MAP_PATH)

# 2) Materyaller
m_floor = make_material("M_Floor", (0.18, 0.12, 0.08), 0.0, 0.6)
m_wall = make_material("M_Wall", (0.55, 0.52, 0.48), 0.0, 0.9)
m_wood = make_material("M_TableWood", (0.25, 0.13, 0.06), 0.0, 0.45)
m_gold = make_material("M_CoinGold", (1.0, 0.72, 0.25), 1.0, 0.25)

# 3) Oda: 600 x 600 x 300 cm, ic yuzeyler z=0 (zemin) ile z=300 (tavan) arasi
W, H, T = 600, 300, 10
box("Floor", (0, 0, -T / 2), (W, W, T), m_floor)
box("Ceiling", (0, 0, H + T / 2), (W, W, T), m_wall)
box("Wall_N", (W / 2 + T / 2, 0, H / 2), (T, W + 2 * T, H), m_wall)
box("Wall_S", (-W / 2 - T / 2, 0, H / 2), (T, W + 2 * T, H), m_wall)
box("Wall_E", (0, W / 2 + T / 2, H / 2), (W, T, H), m_wall)
box("Wall_W", (0, -W / 2 - T / 2, H / 2), (W, T, H), m_wall)

# 4) Masa: ust tabla 160x90x5, yukseklik 80 cm
TOP_Z = 80.0
box("Table_Top", (0, 0, TOP_Z - 2.5), (160, 90, 5), m_wood)
for i, (lx, ly) in enumerate([(72, 37), (72, -37), (-72, 37), (-72, -37)]):
    box(f"Table_Leg_{i + 1}", (lx, ly, (TOP_Z - 5) / 2), (6, 6, TOP_Z - 5), m_wood)

# 5) Coin: cap 6 cm, kalinlik 0.6 cm, masanin ortasinda
COIN_D, COIN_T = 6.0, 0.6
coin = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, TOP_Z + COIN_T / 2), unreal.Rotator(0, 0, 0))
coin.static_mesh_component.set_static_mesh(CYL)
coin.static_mesh_component.set_material(0, m_gold)
coin.set_actor_scale3d(unreal.Vector(COIN_D / 100, COIN_D / 100, COIN_T / 100))
coin.set_actor_label("Coin")
coin.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)

# 6) Isiklar
ceiling = actors.spawn_actor_from_class(unreal.PointLight, unreal.Vector(0, 0, H - 30), unreal.Rotator(0, 0, 0))
ceiling.set_actor_label("Light_Ceiling")
ceiling.light_component.set_intensity(8000.0)
ceiling.light_component.set_attenuation_radius(1200.0)
ceiling.light_component.set_light_color(unreal.LinearColor(1.0, 0.85, 0.65, 1.0))

spot = actors.spawn_actor_from_class(unreal.SpotLight, unreal.Vector(0, 0, 220), unreal.Rotator(roll=0.0, pitch=-90.0, yaw=0.0))
spot.set_actor_label("Light_TableSpot")
spot.spot_light_component.set_intensity(12000.0)
spot.spot_light_component.set_outer_cone_angle(30.0)
spot.spot_light_component.set_inner_cone_angle(15.0)

# 7) Kamera: masanin onunden coin'e bakiyor, oyun basladiginda bu kamera kullanilir
cam_loc = unreal.Vector(-40, 0, 100)
target = unreal.Vector(0, 0, TOP_Z + COIN_T / 2)
d = target - cam_loc
pitch = math.degrees(math.atan2(d.z, math.hypot(d.x, d.y)))
yaw = math.degrees(math.atan2(d.y, d.x))
cam = actors.spawn_actor_from_class(unreal.CameraActor, cam_loc, unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw))
cam.set_actor_label("CoinCamera")
cam.camera_component.set_editor_property("field_of_view", 50.0)
cam.set_editor_property("auto_activate_for_player", unreal.AutoReceiveInput.PLAYER0)

# 8) Kaydet
level_sub.save_current_level()
unreal.log("FlipForFortune: CoinRoom hazir -> " + MAP_PATH)

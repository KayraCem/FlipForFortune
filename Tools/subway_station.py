# Flip for Fortune - kucuk METRO ISTASYONU (masa ve odanin yerine)
# decorate_coin_room.py tarafindan calistirilir (SUBWAY = True). Tek basina calistirma.
# Karakter (-95, 0, 0)'da +X'e bakar. Peron solda, raylar sagda.
#   Ileride: DILEK CESMESI (Ruh Kadehi, Summon), solda evsizin kosesi (Yukseltme),
#   peronun sonunda falci masasi (Enchant), sol kosede CIKIS merdiveni, iki uçta karanlik tuneller.

SUB = ROOT + "/Subway"
SF = "Subway"

X0, X1 = -350.0, 750.0          # peron boyu 11 m
YL, YE, YR = -260.0, 140.0, 360.0   # sol duvar / peron kenari / karsi duvar
HC = 400.0                      # tavan
TZ = -120.0                     # ray cukuru tabani
MIDX, LEN = (X0 + X1) / 2, X1 - X0


# Karakter yururken bu parcalarin icinden gecmez (col=False: kucuk/isik saçan suslemeler)
NO_COL = ("Fountain_Water", "Fountain_Spirit", "Barrel_Flame", "Witch_Candle", "Lamp_", "Exit_Rod", "Homeless_Cup",
          "Homeless_Cardboard", "Platform_Strip", "Platform_Lip", "Poster_", "Sign_Board", "Exit_Sign", "Fountain_Plaque")


def sp(label, mesh, loc, size, mat, rot=None):
    a = part(label, mesh, loc, size, mat, rot=rot, folder=SF)
    if not label.startswith(NO_COL):
        a.static_mesh_component.set_collision_profile_name("BlockAll")
    return a


def sub_light(label, loc, intensity, radius, color, shadows=False):
    l = point_light(label, loc, intensity, radius, color, folder=SF, shadows=shadows)
    return l


# ---------------------------------------------------------------- dokular
TEX = {}
for n in ("Tile", "Floor", "Strip"):
    TEX[n] = import_texture(f"Art/Subway/T_Sub_{n}.png", SUB, f"T_Sub_{n}")
    TEX[n + "_N"] = import_normal(f"Art/Subway/T_Sub_{n}_N.png", SUB, f"T_Sub_{n}_N")
for n in ("Sign", "Exit", "Plaque", "Cardboard", "PosterA", "PosterB", "PosterC"):
    TEX[n] = import_texture(f"Art/Subway/T_Sub_{n}.png", SUB, f"T_Sub_{n}")


def world_mat(name, tex, ntex=None, mode="floor", k=(0.01, 0.01), c=(0.0, 0.0), tint=(1, 1, 1),
              rough=0.7, metallic=0.0, emissive=0.0):
    """Dunya koordinatindan UV: olcek ne olursa olsun doku duzgun dosenir.
    mode: floor (x,y) | wx (x,z: X boyunca uzanan duvar) | wy (y,z)."""
    path = f"{MAT_DIR}/{name}"
    if eal.does_asset_exist(path):
        mat = unreal.load_asset(path)
        mel.delete_all_material_expressions(mat)
    else:
        mat = asset_tools.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    wp = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1400, 0)
    mask = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -1200, 0)
    mask.set_editor_property("r", mode in ("floor", "wx"))
    mask.set_editor_property("g", mode in ("floor", "wy"))
    mask.set_editor_property("b", mode in ("wx", "wy"))
    mel.connect_material_expressions(wp, "", mask, "")
    kk = mel.create_material_expression(mat, unreal.MaterialExpressionConstant2Vector, -1200, 150)
    kk.set_editor_property("r", k[0]); kk.set_editor_property("g", k[1])
    mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -1000, 0)
    mel.connect_material_expressions(mask, "", mul, "A")
    mel.connect_material_expressions(kk, "", mul, "B")
    cc = mel.create_material_expression(mat, unreal.MaterialExpressionConstant2Vector, -1000, 150)
    cc.set_editor_property("r", c[0]); cc.set_editor_property("g", c[1])
    uv = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, -850, 0)
    mel.connect_material_expressions(mul, "", uv, "A")
    mel.connect_material_expressions(cc, "", uv, "B")

    ts = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -650, 0)
    ts.set_editor_property("texture", tex)
    mel.connect_material_expressions(uv, "", ts, "UVs")
    tn = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -650, 250)
    tn.set_editor_property("constant", unreal.LinearColor(tint[0], tint[1], tint[2], 1.0))
    base = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -400, 0)
    mel.connect_material_expressions(ts, "RGB", base, "A")
    mel.connect_material_expressions(tn, "", base, "B")
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    if ntex is not None:
        tsn = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -650, 400)
        tsn.set_editor_property("texture", ntex)
        tsn.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        mel.connect_material_expressions(uv, "", tsn, "UVs")
        mel.connect_material_property(tsn, "RGB", unreal.MaterialProperty.MP_NORMAL)
    r = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 300)
    r.set_editor_property("r", rough)
    mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    m = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 400)
    m.set_editor_property("r", metallic)
    mel.connect_material_property(m, "", unreal.MaterialProperty.MP_METALLIC)
    if emissive > 0:
        e = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 500)
        e.set_editor_property("r", emissive)
        em = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -250, 500)
        mel.connect_material_expressions(base, "", em, "A")
        mel.connect_material_expressions(e, "", em, "B")
        mel.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    return mat


def panel_mat(name, tex, mode, x_left, z_top, w, h, flip=False, emissive=0.0, rough=0.6):
    """Duvardaki tabela/afis: doku tam (x_left..x_left+w, z_top..z_top-h) alanina oturur.
    flip=True: izleyicinin sagi eksenin eksi yonunde (ornegin +Y'ye bakarken sag = -X)."""
    if flip:
        k, c = (-1.0 / w, -1.0 / h), (x_left / w, z_top / h)
    else:
        k, c = (1.0 / w, -1.0 / h), (-x_left / w, z_top / h)
    return world_mat(name, tex, None, mode, k, c, rough=rough, emissive=emissive)


S = {
    "tile": world_mat("M_Sub_TileWall", TEX["Tile"], TEX["Tile_N"], "wx", (0.01, -0.01), tint=(0.95, 0.92, 0.8), rough=0.35),
    "tile_y": world_mat("M_Sub_TileWallY", TEX["Tile"], TEX["Tile_N"], "wy", (0.01, -0.01), tint=(0.95, 0.92, 0.8), rough=0.35),
    "tile_green": world_mat("M_Sub_TileGreen", TEX["Tile"], TEX["Tile_N"], "wx", (0.01, -0.01), tint=(0.12, 0.3, 0.22), rough=0.3),
    "tile_green_y": world_mat("M_Sub_TileGreenY", TEX["Tile"], TEX["Tile_N"], "wy", (0.01, -0.01), tint=(0.12, 0.3, 0.22), rough=0.3),
    "tile_red": world_mat("M_Sub_TileRed", TEX["Tile"], TEX["Tile_N"], "wx", (0.01, -0.01), tint=(0.45, 0.06, 0.07), rough=0.3),
    "floor": world_mat("M_Sub_Floor", TEX["Floor"], TEX["Floor_N"], "floor", (0.005, 0.005), rough=0.55),
    "strip": world_mat("M_Sub_Strip", TEX["Strip"], TEX["Strip_N"], "floor", (0.02, 0.02), rough=0.6),
    "concrete": make_material("M_Sub_Concrete", (0.07, 0.07, 0.068), 0.0, 0.9),
    "void": make_material("M_Sub_Void", (0.0, 0.0, 0.0), 0.0, 1.0),
    "gravel": make_material("M_Sub_Gravel", (0.035, 0.032, 0.03), 0.0, 1.0),
    "rail": make_material("M_Sub_Rail", (0.32, 0.3, 0.28), 1.0, 0.3),
    "tie": make_material("M_Sub_Tie", (0.07, 0.045, 0.03), 0.0, 0.85),
    "steel": make_material("M_Sub_GreenSteel", (0.025, 0.07, 0.05), 0.4, 0.45),
    "tube": make_material("M_Sub_Tube", (0.9, 1.0, 0.95), 0.0, 0.5, emissive=(9.0, 11.0, 10.0)),
    "tube_dead": make_material("M_Sub_TubeDead", (0.5, 0.55, 0.5), 0.0, 0.5, emissive=(0.15, 0.18, 0.16)),
    "stone": make_material("M_Sub_Stone", (0.16, 0.155, 0.15), 0.0, 0.75),
    "stone_dark": make_material("M_Sub_StoneDark", (0.06, 0.06, 0.065), 0.0, 0.7),
    "water": make_material("M_Sub_Water", (0.05, 0.4, 0.42), 0.0, 0.05, emissive=(0.05, 0.5, 0.45), opacity=0.45),
    "spirit": make_material("M_Sub_SpiritFlame", (0.2, 1.0, 0.85), 0.0, 1.0, emissive=(1.5, 12.0, 9.0)),
    "cardboard": make_material("M_Sub_Cardboard", (0.36, 0.25, 0.13), 0.0, 0.95),
    "blanket": make_material("M_Sub_Blanket", (0.09, 0.06, 0.05), 0.0, 1.0),
    "rust": make_material("M_Sub_Rust", (0.2, 0.08, 0.03), 0.6, 0.8),
    "cloth": make_material("M_Sub_WitchCloth", (0.09, 0.02, 0.14), 0.0, 0.9),
    "curtain": make_material("M_Sub_Curtain", (0.06, 0.01, 0.08), 0.0, 1.0),
    "crystal": make_material("M_Sub_Crystal", (0.6, 0.3, 1.0), 0.0, 0.05, emissive=(3.0, 1.0, 7.0)),
    "signal": make_material("M_Sub_SignalRed", (1.0, 0.05, 0.03), 0.0, 0.4, emissive=(30.0, 1.0, 0.5)),
    "wood": M["chest"], "iron": M["iron"], "gold": M["gold"], "flame": M["flame"], "candle": M["candle"],
}

# ---------------------------------------------------------------- peron + ray cukuru
sp("Platform", CUBE, (MIDX, (YL + YE) / 2, -62), (LEN, YE - YL, 124), S["floor"])
sp("Platform_Strip", CUBE, (MIDX, YE - 26, 0.6), (LEN, 40, 1.2), S["strip"])
sp("Platform_Lip", CUBE, (MIDX, YE - 3, 1.0), (LEN, 6, 2), S["concrete"])
sp("Platform_Face", CUBE, (MIDX, YE + 1, -60), (LEN, 2, 120), S["concrete"])
# Gorunmez duvar: karakter raylara dusmesin
edge = sp("Platform_EdgeBlocker", CUBE, (MIDX, YE + 6, 100), (LEN, 10, 200), S["concrete"])
edge.set_actor_hidden_in_game(True)
edge.static_mesh_component.set_collision_response_to_channel(unreal.CollisionChannel.ECC_VISIBILITY, unreal.CollisionResponseType.ECR_IGNORE)

TX0, TX1 = X0 - 600, X1 + 600          # tuneller dahil
sp("Track_Bed", CUBE, (MIDX, (YE + YR) / 2, TZ - 5), (TX1 - TX0, YR - YE, 10), S["gravel"])
for i, ry in enumerate((210.0, 290.0)):
    sp(f"Rail_{i}", CUBE, (MIDX, ry, TZ + 8), (TX1 - TX0, 7, 12), S["rail"])
sp("Rail_Third", CUBE, (MIDX, 335, TZ + 10), (TX1 - TX0, 10, 10), S["iron"])
x = TX0 + 20
i = 0
while x < TX1:
    sp(f"Tie_{i}", CUBE, (x, 250, TZ + 2), (24, 180, 8), S["tie"])
    x += 62
    i += 1

# ---------------------------------------------------------------- duvarlar + tavan
sp("Wall_Left", CUBE, (MIDX, YL - 10, HC / 2), (LEN, 20, HC), S["tile"])
sp("Wall_Left_Band", CUBE, (MIDX, YL + 1, 55), (LEN, 2, 110), S["tile_green"])
sp("Wall_Left_Trim", CUBE, (MIDX, YL + 1, 150), (LEN, 2, 12), S["tile_red"])
sp("Wall_Far", CUBE, (MIDX, YR + 10, (HC + TZ) / 2), (LEN, 20, HC - TZ), S["tile"])
sp("Wall_Far_Band", CUBE, (MIDX, YR - 1, 45), (LEN, 2, 90), S["tile_green"])
sp("Wall_Far_Low", CUBE, (MIDX, YR - 1, TZ / 2), (LEN, 2, -TZ), S["concrete"])
sp("Wall_Far_Trim", CUBE, (MIDX, YR - 1, 150), (LEN, 2, 12), S["tile_red"])
sp("Ceiling", CUBE, (MIDX, (YL + YR) / 2, HC + 10), (LEN + 40, YR - YL + 40, 20), S["concrete"])
for i, bx in enumerate(range(int(X0) + 100, int(X1), 220)):
    sp(f"Ceiling_Beam_{i}", CUBE, (bx, (YL + YR) / 2, HC - 14), (22, YR - YL, 28), S["steel"])

# Bati ucu (karakterin arkasi): duz duvar + tunel
sp("End_W", CUBE, (X0 - 10, (YL + YE) / 2, HC / 2), (20, YE - YL, HC), S["tile_y"])
sp("End_W_Band", CUBE, (X0 + 1, (YL + YE) / 2, 55), (2, YE - YL, 110), S["tile_green_y"])

# Dogu ucu: CIKIS kapisi (sol) + duvar
DY0, DY1, DZ = YL, -150.0, 230.0
sp("End_E", CUBE, (X1 + 10, (DY1 + YE) / 2, HC / 2), (20, YE - DY1, HC), S["tile_y"])
sp("End_E_Band", CUBE, (X1 - 1, (DY1 + YE) / 2, 55), (2, YE - DY1, 110), S["tile_green_y"])
sp("End_E_Lintel", CUBE, (X1 + 10, (DY0 + DY1) / 2, (DZ + HC) / 2), (20, DY1 - DY0, HC - DZ), S["tile_y"])
sp("Door_Frame_Top", CUBE, (X1 - 2, (DY0 + DY1) / 2, DZ + 4), (6, DY1 - DY0, 8), S["steel"])
sp("Door_Frame_Side", CUBE, (X1 - 2, DY1 - 4, DZ / 2), (6, 8, DZ), S["steel"])
# Merdiven: kapidan yukari (sokaga) cikar
for s in range(10):
    sp(f"Stair_{s}", CUBE, (X1 + 35 + s * 30, (DY0 + DY1) / 2, (s + 1) * 9), (30, DY1 - DY0, (s + 1) * 18), S["concrete"])
    sp(f"Stair_Nose_{s}", CUBE, (X1 + 21 + s * 30, (DY0 + DY1) / 2, (s + 1) * 18 + 0.5), (3, DY1 - DY0, 1), S["strip"])
SX1 = X1 + 20 + 10 * 30 + 60
sp("Stairwell_L", CUBE, ((X1 + SX1) / 2, DY0 - 10, HC / 2 + 90), (SX1 - X1, 20, HC + 180), S["tile"])
sp("Stairwell_R", CUBE, ((X1 + SX1) / 2, DY1 + 10, HC / 2 + 90), (SX1 - X1, 20, HC + 180), S["tile"])
sp("Stairwell_Top", CUBE, (SX1 - 30, (DY0 + DY1) / 2, 180 - 5), (60, DY1 - DY0, 10), S["concrete"])
sp("Stairwell_End", CUBE, (SX1 + 10, (DY0 + DY1) / 2, HC / 2 + 90), (20, DY1 - DY0, HC + 180), S["void"])
sp("Stairwell_Ceil", CUBE, ((X1 + SX1) / 2, (DY0 + DY1) / 2, HC + 180), (SX1 - X1, DY1 - DY0 + 40, 20), S["concrete"])
# Gorunmez duvarlar: peronun etrafi + merdiven kapisi (karakter raylara inmez, duvardan gecmez)
def blocker(label, loc, size):
    b = sp(label, CUBE, loc, size, S["concrete"])
    b.set_actor_hidden_in_game(True)
    b.static_mesh_component.set_collision_response_to_channel(unreal.CollisionChannel.ECC_VISIBILITY, unreal.CollisionResponseType.ECR_IGNORE)
    return b
blocker("Blocker_Track", (MIDX, YE + 15, 150), (LEN + 200, 30, 300))
blocker("Blocker_Left", (MIDX, YL - 15, 150), (LEN + 200, 30, 300))
blocker("Blocker_West", (X0 - 15, (YL + YE) / 2, 150), (30, YE - YL + 60, 300))
blocker("Blocker_East", (X1 + 15, (YL + YE) / 2, 150), (30, YE - YL + 60, 300))   # merdiven kapisi dahil
sub_light("Light_Street", (SX1 - 60, (DY0 + DY1) / 2, 360), 1200.0, 450.0, (1.0, 0.6, 0.3))

# Tuneller (iki uc): karanlik agiz + kirmizi sinyal
for side, (ex, dirx) in enumerate(((X1, 1.0), (X0, -1.0))):
    tag = "E" if dirx > 0 else "W"
    sp(f"Tunnel_{tag}_Over", CUBE, (ex + dirx * 10, (YE + YR) / 2, (170 + HC) / 2), (20, YR - YE + 20, HC - 170), S["tile_y"])
    sp(f"Tunnel_{tag}_Arch", CUBE, (ex - dirx * 3, (YE + YR) / 2, 165), (8, YR - YE, 12), S["steel"])
    cx = ex + dirx * 300
    sp(f"Tunnel_{tag}_L", CUBE, (cx, YE - 5, (TZ + 170) / 2), (600, 10, 170 - TZ), S["concrete"])
    sp(f"Tunnel_{tag}_R", CUBE, (cx, YR + 5, (TZ + 170) / 2), (600, 10, 170 - TZ), S["concrete"])
    sp(f"Tunnel_{tag}_Top", CUBE, (cx, (YE + YR) / 2, 175), (600, YR - YE + 20, 10), S["concrete"])
    sp(f"Tunnel_{tag}_Back", CUBE, (ex + dirx * 600, (YE + YR) / 2, (TZ + 170) / 2), (10, YR - YE, 170 - TZ), S["void"])
    sp(f"Signal_{tag}_Box", CUBE, (ex + dirx * 160, YE + 8, 10), (14, 10, 40), S["iron"])
    sp(f"Signal_{tag}_Lamp", SPH, (ex + dirx * 160 - dirx * 7.5, YE + 8, 20), (5, 5, 5), S["signal"])
    sub_light(f"Light_Signal_{tag}", (ex + dirx * 150, YE + 20, 20), 220.0, 260.0, (1.0, 0.05, 0.02))

# ---------------------------------------------------------------- kolonlar, lambalar, banklar
for i, px in enumerate((60.0, 400.0)):
    py = YE - 75
    sp(f"Pillar_{i}", CUBE, (px, py, HC / 2), (26, 26, HC), S["steel"])
    sp(f"Pillar_{i}_Base", CUBE, (px, py, 12), (40, 40, 24), S["steel"])
    sp(f"Pillar_{i}_Cap", CUBE, (px, py, HC - 40), (36, 36, 14), S["steel"])
    for j, fy in enumerate((-13.5, 13.5)):
        sp(f"Pillar_{i}_Flange_{j}", CUBE, (px, py + fy, HC / 2), (40, 3, HC), S["steel"])

LAMPS = [(-120.0, True), (180.0, True), (480.0, False), (640.0, True)]   # biri bozuk
for i, (lx, on) in enumerate(LAMPS):
    ly, lz = -50.0, HC - 70
    for j, ry in enumerate((-50.0, 50.0)):
        sp(f"Lamp_{i}_Rod_{j}", CYL, (lx + ry, ly, lz + 35), (2, 2, 70), S["iron"])
    sp(f"Lamp_{i}_Housing", CUBE, (lx, ly, lz + 4), (130, 22, 8), S["steel"])
    for j, ty in enumerate((-5.0, 5.0)):
        sp(f"Lamp_{i}_Tube_{j}", CYL, (lx, ly + ty, lz - 1), (4, 4, 120), S["tube"] if on else S["tube_dead"], rot=R(pitch=90))
    if on:
        sub_light(f"Light_Lamp_{i}", (lx, ly, lz - 20), 2600.0, 750.0, (0.78, 0.95, 0.85), shadows=(i % 2 == 0))
sub_light("Light_Track", (300.0, 260.0, 250.0), 700.0, 500.0, (0.6, 0.75, 0.9))

for i, bx in enumerate((-230.0, 620.0)):
    sp(f"Bench_{i}_Seat", CUBE, (bx, YL + 28, 45), (140, 40, 6), S["wood"])
    sp(f"Bench_{i}_Back", CUBE, (bx, YL + 8, 75), (140, 6, 40), S["wood"])
    for j, dx in enumerate((-60.0, 60.0)):
        sp(f"Bench_{i}_Leg_{j}", CUBE, (bx + dx, YL + 25, 21), (6, 34, 42), S["iron"])
sp("Trash_Can", CYL, (500.0, YL + 25, 38), (40, 40, 76), S["steel"])
sp("Trash_Lid", CYL, (500.0, YL + 25, 78), (44, 44, 4), S["iron"])

# ---------------------------------------------------------------- tabela, afisler, cikis isareti
SG_X, SG_W, SG_H, SG_Z = 300.0, 380.0, 95.0, 235.0     # karsi duvarda istasyon adi
sp("Sign_Back", CUBE, (SG_X, YR - 2, SG_Z), (SG_W + 16, 3, SG_H + 16), S["iron"])
sp("Sign_Board", CUBE, (SG_X, YR - 4, SG_Z), (SG_W, 2, SG_H),
   panel_mat("M_Sub_Sign", TEX["Sign"], "wx", SG_X + SG_W / 2, SG_Z + SG_H / 2, SG_W, SG_H, flip=True, emissive=0.35))
sub_light("Light_Sign", (SG_X, YR - 70, SG_Z + 90), 900.0, 300.0, (1.0, 0.85, 0.6))

for i, (name, px, pw, ph, pz) in enumerate((("PosterA", 520.0, 80.0, 120.0, 180.0),
                                             ("PosterC", 660.0, 80.0, 120.0, 180.0),
                                             ("PosterB", 210.0, 80.0, 120.0, 180.0))):
    sp(f"Poster_{name}", CUBE, (px, YL + 2.5, pz), (pw, 1, ph),
       panel_mat(f"M_Sub_{name}", TEX[name], "wx", px - pw / 2, pz + ph / 2, pw, ph))

EX_Y, EX_W, EX_H, EX_Z, EX_X = (DY0 + DY1) / 2, 110.0, 41.0, 300.0, 560.0   # tavandan sarkan CIKIS
for j, dy in enumerate((-45.0, 45.0)):
    sp(f"Exit_Rod_{j}", CYL, (EX_X, EX_Y + dy, (EX_Z + HC) / 2), (1.5, 1.5, HC - EX_Z), S["iron"])
sp("Exit_Box", CUBE, (EX_X + 3, EX_Y, EX_Z), (6, EX_W + 6, EX_H + 6), S["iron"])
sp("Exit_Sign", CUBE, (EX_X - 1, EX_Y, EX_Z), (2, EX_W, EX_H),
   panel_mat("M_Sub_Exit", TEX["Exit"], "wy", EX_Y - EX_W / 2, EX_Z + EX_H / 2, EX_W, EX_H, emissive=3.0))
sub_light("Light_Exit", (EX_X - 30, EX_Y, EX_Z - 20), 150.0, 220.0, (0.3, 1.0, 0.5))

# ---------------------------------------------------------------- DILEK CESMESI = SUMMON ("Ruh Kadehi")
FX, FY = 230.0, -20.0
WATER_Z = 48.0      # havuz suyu (cesme tabanina gore)


def fnode(mat, cls, x, y, **props):
    e = mel.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def flink(a, b, inp, out=""):
    mel.connect_material_expressions(a, out, b, inp)


def fresh_mat(name):
    path = f"{MAT_DIR}/{name}"
    if eal.does_asset_exist(path):
        mat = unreal.load_asset(path)
        mel.delete_all_material_expressions(mat)
    else:
        mat = asset_tools.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    return mat


def fountain_materials():
    tex = import_texture("Art/Fountain/T_Fountain_Stone.png", SUB, "T_Fountain_Stone")
    ntex = import_normal("Art/Fountain/T_Fountain_Stone_N.png", SUB, "T_Fountain_Stone_N")
    MP = unreal.MaterialProperty

    # --- tas: yontulmus tas dokusu (yanlar + ustler), renk = vertex color, altin kisimlar metal
    m = fresh_mat("M_Fountain_Stone")
    wp = fnode(m, unreal.MaterialExpressionWorldPosition, -1800, 0)
    mx = fnode(m, unreal.MaterialExpressionComponentMask, -1600, -100, r=True, g=False, b=False)
    my = fnode(m, unreal.MaterialExpressionComponentMask, -1600, 0, r=False, g=True, b=False)
    mz = fnode(m, unreal.MaterialExpressionComponentMask, -1600, 100, r=False, g=False, b=True)
    mxy = fnode(m, unreal.MaterialExpressionComponentMask, -1600, 200, r=True, g=True, b=False)
    for e in (mx, my, mz, mxy):
        flink(wp, e, "")
    sxy = fnode(m, unreal.MaterialExpressionAdd, -1400, -50); flink(mx, sxy, "A"); flink(my, sxy, "B")
    nz = fnode(m, unreal.MaterialExpressionMultiply, -1400, 100, const_b=-1.0); flink(mz, nz, "A")
    side = fnode(m, unreal.MaterialExpressionAppendVector, -1250, 0); flink(sxy, side, "A"); flink(nz, side, "B")
    side_uv = fnode(m, unreal.MaterialExpressionMultiply, -1100, 0, const_b=0.012); flink(side, side_uv, "A")
    top_uv = fnode(m, unreal.MaterialExpressionMultiply, -1100, 200, const_b=0.012); flink(mxy, top_uv, "A")
    ts = fnode(m, unreal.MaterialExpressionTextureSample, -900, -100, texture=tex); flink(side_uv, ts, "UVs")
    tt = fnode(m, unreal.MaterialExpressionTextureSample, -900, 150, texture=tex); flink(top_uv, tt, "UVs")
    nrm = fnode(m, unreal.MaterialExpressionVertexNormalWS, -1100, 400)
    nzm = fnode(m, unreal.MaterialExpressionComponentMask, -950, 400, r=False, g=False, b=True); flink(nrm, nzm, "")
    nza = fnode(m, unreal.MaterialExpressionAbs, -800, 400); flink(nzm, nza, "")
    blend = fnode(m, unreal.MaterialExpressionPower, -650, 400, const_exponent=3.0); flink(nza, blend, "Base")
    lt = fnode(m, unreal.MaterialExpressionLinearInterpolate, -650, 0)
    flink(ts, lt, "A", "RGB"); flink(tt, lt, "B", "RGB"); flink(blend, lt, "Alpha")
    vc = fnode(m, unreal.MaterialExpressionVertexColor, -650, -250)
    tint = fnode(m, unreal.MaterialExpressionMultiply, -450, -100); flink(lt, tint, "A"); flink(vc, tint, "B", "")
    boost = fnode(m, unreal.MaterialExpressionMultiply, -300, -100, const_b=1.3); flink(tint, boost, "A")
    mel.connect_material_property(boost, "", MP.MP_BASE_COLOR)
    # altin = kirmizi >> mavi
    rb = fnode(m, unreal.MaterialExpressionSubtract, -450, -350); flink(vc, rb, "A", "R"); flink(vc, rb, "B", "B")
    rb3 = fnode(m, unreal.MaterialExpressionMultiply, -300, -350, const_b=3.0); flink(rb, rb3, "A")
    metal = fnode(m, unreal.MaterialExpressionSaturate, -150, -350); flink(rb3, metal, "")
    mel.connect_material_property(metal, "", MP.MP_METALLIC)
    rough = fnode(m, unreal.MaterialExpressionLinearInterpolate, -150, -250, const_a=0.82, const_b=0.32); flink(metal, rough, "Alpha")
    mel.connect_material_property(rough, "", MP.MP_ROUGHNESS)
    tn = fnode(m, unreal.MaterialExpressionTextureSample, -900, 600, texture=ntex, sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    flink(side_uv, tn, "UVs")
    mel.connect_material_property(tn, "RGB", MP.MP_NORMAL)
    mel.recompile_material(m); eal.save_loaded_asset(m)

    # --- runler + ruh kuresi: nabiz gibi parlar
    m = fresh_mat("M_Fountain_Glow")
    vc = fnode(m, unreal.MaterialExpressionVertexColor, -800, 0)
    tm = fnode(m, unreal.MaterialExpressionTime, -1000, 200)
    t2 = fnode(m, unreal.MaterialExpressionMultiply, -850, 200, const_b=2.0); flink(tm, t2, "A")
    sn = fnode(m, unreal.MaterialExpressionSine, -700, 200, period=6.2832); flink(t2, sn, "")
    pulse = fnode(m, unreal.MaterialExpressionMultiply, -550, 200, const_b=6.0); flink(sn, pulse, "A")
    amp = fnode(m, unreal.MaterialExpressionAdd, -400, 200, const_b=22.0); flink(pulse, amp, "A")
    em = fnode(m, unreal.MaterialExpressionMultiply, -250, 0); flink(vc, em, "A", ""); flink(amp, em, "B")
    mel.connect_material_property(em, "", MP.MP_EMISSIVE_COLOR)
    blk = fnode(m, unreal.MaterialExpressionConstant3Vector, -250, -150, constant=unreal.LinearColor(0.02, 0.05, 0.05, 1))
    mel.connect_material_property(blk, "", MP.MP_BASE_COLOR)
    mel.recompile_material(m); eal.save_loaded_asset(m)

    # --- su yuzeyi: yari saydam, isiltili, halka halka dalgalanir (WPO)
    m = fresh_mat("M_Fountain_Water")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    wp = fnode(m, unreal.MaterialExpressionWorldPosition, -1600, 0)
    tm = fnode(m, unreal.MaterialExpressionTime, -1600, 300)
    mx = fnode(m, unreal.MaterialExpressionComponentMask, -1400, -100, r=True, g=False, b=False); flink(wp, mx, "")
    my = fnode(m, unreal.MaterialExpressionComponentMask, -1400, 0, r=False, g=True, b=False); flink(wp, my, "")
    ax = fnode(m, unreal.MaterialExpressionMultiply, -1250, -100, const_b=0.15); flink(mx, ax, "A")
    ay = fnode(m, unreal.MaterialExpressionMultiply, -1250, 0, const_b=0.13); flink(my, ay, "A")
    t1 = fnode(m, unreal.MaterialExpressionMultiply, -1250, 300, const_b=1.4); flink(tm, t1, "A")
    t3 = fnode(m, unreal.MaterialExpressionMultiply, -1250, 400, const_b=-1.1); flink(tm, t3, "A")
    p1 = fnode(m, unreal.MaterialExpressionAdd, -1100, -100); flink(ax, p1, "A"); flink(t1, p1, "B")
    p2 = fnode(m, unreal.MaterialExpressionAdd, -1100, 0); flink(ay, p2, "A"); flink(t3, p2, "B")
    s1 = fnode(m, unreal.MaterialExpressionSine, -950, -100, period=6.2832); flink(p1, s1, "")
    s2 = fnode(m, unreal.MaterialExpressionSine, -950, 0, period=6.2832); flink(p2, s2, "")
    sh = fnode(m, unreal.MaterialExpressionMultiply, -800, -50); flink(s1, sh, "A"); flink(s2, sh, "B")
    sh2 = fnode(m, unreal.MaterialExpressionMultiply, -650, -50, const_b=1.4); flink(sh, sh2, "A")
    lum = fnode(m, unreal.MaterialExpressionAdd, -500, -50, const_b=2.6); flink(sh2, lum, "A")
    col = fnode(m, unreal.MaterialExpressionConstant3Vector, -500, -200, constant=unreal.LinearColor(0.05, 0.42, 0.38, 1))
    em = fnode(m, unreal.MaterialExpressionMultiply, -300, -100); flink(col, em, "A"); flink(lum, em, "B")
    mel.connect_material_property(em, "", MP.MP_EMISSIVE_COLOR)
    op = fnode(m, unreal.MaterialExpressionConstant, -300, 50, r=0.86)
    mel.connect_material_property(op, "", MP.MP_OPACITY)
    # halka dalga: z += sin(merkeze uzaklik * 0.35 - zaman * 3) * 0.35
    ob = fnode(m, unreal.MaterialExpressionObjectPositionWS, -1600, 600)
    ds = fnode(m, unreal.MaterialExpressionDistance, -1400, 600); flink(wp, ds, "A"); flink(ob, ds, "B")
    dk = fnode(m, unreal.MaterialExpressionMultiply, -1250, 600, const_b=0.35); flink(ds, dk, "A")
    tw = fnode(m, unreal.MaterialExpressionMultiply, -1250, 700, const_b=-3.0); flink(tm, tw, "A")
    ph = fnode(m, unreal.MaterialExpressionAdd, -1100, 600); flink(dk, ph, "A"); flink(tw, ph, "B")
    sw = fnode(m, unreal.MaterialExpressionSine, -950, 600, period=6.2832); flink(ph, sw, "")
    hz = fnode(m, unreal.MaterialExpressionMultiply, -800, 600, const_b=0.35); flink(sw, hz, "A")
    z0 = fnode(m, unreal.MaterialExpressionConstant2Vector, -800, 500, r=0.0, g=0.0)
    wpo = fnode(m, unreal.MaterialExpressionAppendVector, -650, 550); flink(z0, wpo, "A"); flink(hz, wpo, "B")
    mel.connect_material_property(wpo, "", MP.MP_WORLD_POSITION_OFFSET)
    mel.recompile_material(m); eal.save_loaded_asset(m)

    # --- dokulen su: yukaridan asagi akan parlak seritler (UV.y = akis)
    m = fresh_mat("M_Fountain_Falls")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    uv = fnode(m, unreal.MaterialExpressionTextureCoordinate, -1300, 0)
    vy = fnode(m, unreal.MaterialExpressionComponentMask, -1150, 0, r=False, g=True); flink(uv, vy, "")
    v6 = fnode(m, unreal.MaterialExpressionMultiply, -1000, 0, const_b=3.0); flink(vy, v6, "A")
    tm = fnode(m, unreal.MaterialExpressionTime, -1150, 200)
    tf = fnode(m, unreal.MaterialExpressionMultiply, -1000, 200, const_b=-1.6); flink(tm, tf, "A")
    ph = fnode(m, unreal.MaterialExpressionAdd, -850, 100); flink(v6, ph, "A"); flink(tf, ph, "B")
    # yumusak dalga (keskin "kesik" cizgi olmasin): 0.5 + 0.5 * sin
    fr = fnode(m, unreal.MaterialExpressionSine, -760, 100, period=1.0); flink(ph, fr, "")
    hs = fnode(m, unreal.MaterialExpressionMultiply, -640, 100, const_b=0.5); flink(fr, hs, "A")
    hs2 = fnode(m, unreal.MaterialExpressionAdd, -540, 100, const_b=0.5); flink(hs, hs2, "A")
    pw = fnode(m, unreal.MaterialExpressionPower, -440, 100, const_exponent=2.0); flink(hs2, pw, "Base")
    br = fnode(m, unreal.MaterialExpressionMultiply, -340, 100, const_b=2.2); flink(pw, br, "A")
    b2 = fnode(m, unreal.MaterialExpressionAdd, -250, 100, const_b=3.0); flink(br, b2, "A")
    col = fnode(m, unreal.MaterialExpressionConstant3Vector, -400, -100, constant=unreal.LinearColor(0.3, 1.0, 0.88, 1))
    em = fnode(m, unreal.MaterialExpressionMultiply, -100, 0); flink(col, em, "A"); flink(b2, em, "B")
    mel.connect_material_property(em, "", MP.MP_EMISSIVE_COLOR)
    op = fnode(m, unreal.MaterialExpressionConstant, -100, 200, r=0.85)
    mel.connect_material_property(op, "", MP.MP_OPACITY)
    mel.recompile_material(m); eal.save_loaded_asset(m)


try:
    fountain_materials()
except Exception as e:
    unreal.log_warning(f"Cesme materyalleri: {e}")

try:
    ftn = actors.spawn_actor_from_class(unreal.FortuneFountain, V(FX, FY, 0), R())
    ftn.set_actor_label("Fountain_RuhKadehi")
    ftn.set_folder_path(SF)
except Exception as e:
    unreal.log_warning(f"Cesme eklenemedi (projeyi derledin mi?): {e}")

# Yurume carpismasi: cesmenin etrafinda gorunmez silindir (nisangahi engellemez)
fb = sp("Fountain_Blocker", CYL, (FX, FY, 100), (196, 196, 200), S["concrete"])
fb.set_actor_hidden_in_game(True)
fb.static_mesh_component.set_collision_response_to_channel(unreal.CollisionChannel.ECC_VISIBILITY, unreal.CollisionResponseType.ECR_IGNORE)

# Dipte oyundaki gercek coinler: akintiyla sallanir, birkaci dipten yukselip suyun ustunde doner
try:
    pile = actors.spawn_actor_from_class(unreal.FortuneCoinPile, V(FX, FY, 21), R())
    pile.set_actor_label("Fountain_CoinPile")
    pile.set_folder_path(SF)
    pile.set_editor_property("radius", 66.0)
    pile.set_editor_property("count", 80)
    pile.set_editor_property("floating_count", 5)
    pile.set_editor_property("float_height", 40.0)
    pile.set_editor_property("riser_count", 4)
    pile.set_editor_property("rise_height", 42.0)
    pile2 = actors.spawn_actor_from_class(unreal.FortuneCoinPile, V(FX, FY, 105.5), R())   # 2. kattaki kase
    pile2.set_actor_label("Fountain_CoinPile_Top")
    pile2.set_folder_path(SF)
    pile2.set_editor_property("radius", 36.0)
    pile2.set_editor_property("count", 16)
    pile2.set_editor_property("mound_height", 2.0)
    pile2.set_editor_property("floating_count", 0)
    pile2.set_editor_property("riser_count", 1)
    pile2.set_editor_property("rise_height", 26.0)
    pile2.set_editor_property("coin_diameter", 6.0)
except Exception as e:
    unreal.log_warning(f"Coin yigini eklenemedi (projeyi derledin mi?): {e}")

# Sandik gorunmez: cesmeye tiklayinca Summon acilir, coinler 2. kattan yukselir
try:
    gacha.set_actor_location(V(FX, FY, 95), False, False)
    gacha.set_actor_rotation(R(), False)
    gacha.set_actor_scale3d(V(1, 1, 1))
    gacha.set_actor_label("Fountain_Summon")
    gacha.set_editor_property("label", unreal.Text("Dilek Çeşmesi"))
    gacha.set_editor_property("hide_model", True)
    gacha.set_editor_property("click_extent", V(100, 100, 95))
except Exception as e:
    unreal.log_warning(f"Cesme summon ayarlanamadi (projeyi derledin mi?): {e}")

# ---------------------------------------------------------------- SUMMON SAHNESI (ayri oda)
# Summon'a basinca kamera buraya gecer: cesmenin kopyasi, karanlik magara, isik huzmeleri. Bitince metroya doner.
GX, GY = 0.0, 6000.0
GF = "Subway/SummonStage"


def gp(label, mesh, loc, size, mat, rot=None):
    return part(label, mesh, loc, size, mat, rot=rot, folder=GF)


def g_light(label, loc, intensity, radius, color):
    l = point_light(label, loc, intensity, radius, color, folder=GF, shadows=False)
    return l


gp("Stage_Floor", CYL, (GX, GY, -5), (1000, 1000, 10), S["floor"])
gp("Stage_Dais", CYL, (GX, GY, 3), (300, 300, 6), S["stone_dark"])
gp("Stage_DaisRing", CYL, (GX, GY, 6.3), (312, 312, 1), S["gold"])
for i in range(10):                                  # cevrede yuksek tas kolonlar + ruh alevleri
    a = math.radians(i * 36 + 18)
    px, py = GX + math.cos(a) * 340, GY + math.sin(a) * 340
    gp(f"Stage_Pillar_{i}", CYL, (px, py, 180), (46, 46, 360), S["stone"])
    gp(f"Stage_PillarCap_{i}", CUBE, (px, py, 365), (60, 60, 14), S["stone_dark"], rot=R(yaw=i * 36))
    gp(f"Stage_PillarBand_{i}", CYL, (px, py, 120), (50, 50, 3), S["gold"])
    gp(f"Stage_Brazier_{i}", CYL, (px, py, 380), (34, 34, 10), S["iron"])
    gp(f"Stage_Spirit_{i}", SPH, (px, py, 395), (16, 16, 26), S["spirit"])

try:
    sf = actors.spawn_actor_from_class(unreal.FortuneFountain, V(GX, GY, 6), R())
    sf.set_actor_label("Stage_Fountain")
    sf.set_folder_path(GF)
    sp1 = actors.spawn_actor_from_class(unreal.FortuneCoinPile, V(GX, GY, 27), R())
    sp1.set_actor_label("Stage_CoinPile")
    sp1.set_folder_path(GF)
    for k, v in (("radius", 66.0), ("count", 90), ("floating_count", 6), ("float_height", 40.0), ("riser_count", 5), ("rise_height", 42.0)):
        sp1.set_editor_property(k, v)
    stage_chest = actors.spawn_actor_from_class(unreal.FortuneGachaChest, V(GX, GY, 101), R())
    stage_chest.set_actor_label("Stage_SummonChest")
    stage_chest.set_folder_path(GF)
    stage_chest.set_editor_property("hide_model", True)
    stage_chest.set_editor_property("label", unreal.Text("Dilek Çeşmesi"))
    stage_chest.set_editor_property("emerge_offset", V(-62, 0, -47))   # coinler cesmenin on tarafindaki sudan cikar
    stage_chest.set_editor_property("emerge_spread", 42.0)
    stage_chest.set_editor_property("linked_fountain", sf)

    stage_cam = actors.spawn_actor_from_class(unreal.CameraActor, V(GX - 340, GY, 150), R())
    tgt = V(GX, GY, 108)
    d = tgt - stage_cam.get_actor_location()
    stage_cam.set_actor_rotation(R(pitch=math.degrees(math.atan2(d.z, math.hypot(d.x, d.y))), yaw=0.0), False)
    stage_cam.set_actor_label("SummonCamera")
    stage_cam.set_folder_path(GF)
    stage_cam.camera_component.set_editor_property("field_of_view", 62.0)
    stage_cam.set_editor_property("tags", [unreal.Name("SummonCamera")])
    stage_cam.set_editor_property("auto_activate_for_player", unreal.AutoReceiveInput.DISABLED)

    stage_chest.set_editor_property("stage_camera", stage_cam)
    gacha.set_editor_property("stage_chest", stage_chest)     # metrodaki cesme -> summon burada oynar
except Exception as e:
    unreal.log_warning(f"Summon sahnesi kurulamadi (projeyi derledin mi?): {e}")

# Isiklar: yukaridan beyaz-turkuaz huzme, arkadan mor kenar isiklari, kolon alevleri
spot = actors.spawn_actor_from_class(unreal.SpotLight, V(GX - 60, GY, 470), R(pitch=-80))
spot.set_actor_label("Stage_Spot")
spot.set_folder_path(GF)
spot.spot_light_component.set_intensity(16000.0)
spot.spot_light_component.set_outer_cone_angle(26.0)
spot.spot_light_component.set_inner_cone_angle(12.0)
spot.spot_light_component.set_light_color(unreal.LinearColor(0.75, 1.0, 0.95, 1.0))
for i, (dx, dy) in enumerate(((220, -200), (220, 200), (-120, -260), (-120, 260))):
    g_light(f"Stage_Rim_{i}", (GX + dx, GY + dy, 160), 2500.0, 480.0, (0.55, 0.25, 1.0) if i < 2 else (0.2, 0.9, 0.8))
for i in range(10):
    a = math.radians(i * 36 + 18)
    g_light(f"Stage_SpiritLight_{i}", (GX + math.cos(a) * 340, GY + math.sin(a) * 340, 400), 600.0, 300.0, (0.25, 1.0, 0.85))

# ---------------------------------------------------------------- EVSIZIN KOSESI = YUKSELTMELER
# (Karakter modeli onaylaninca battaniyenin yerine gelecek)
HX, HY = 340.0, YL + 45
up = interactable("Click_Upgrades", unreal.FortuneInteraction.UPGRADES, "Yükseltmeler", (HX, HY, 40), (55, 45, 45))
up.set_folder_path(SF)
sp("Homeless_Cardboard", CUBE, (HX, HY, 1), (110, 70, 2), S["cardboard"], rot=R(yaw=4))
sp("Homeless_Blanket", SPH, (HX + 5, HY - 8, 22), (75, 48, 44), S["blanket"])
sp("Homeless_Bag", CUBE, (HX - 45, HY - 20, 18), (28, 22, 36), S["tie"], rot=R(yaw=-12))
sp("Homeless_Cup", CYL, (HX - 20, HY + 32, 6), (8, 8, 12), S["rail"])
sp("Homeless_CupCoin", CYL, (HX - 20, HY + 32, 12.5), (4.5, 4.5, 0.5), S["gold"], rot=R(roll=12))
SGN_W, SGN_H, SGN_Z = 64.0, 48.0, 58.0
sp("Homeless_Sign", CUBE, (HX + 10, YL + 3, SGN_Z), (SGN_W, 1.5, SGN_H),
   panel_mat("M_Sub_Cardboard", TEX["Cardboard"], "wx", HX + 10 - SGN_W / 2, SGN_Z + SGN_H / 2, SGN_W, SGN_H, rough=0.95))
BX, BY = 430.0, YL + 70                       # varil atesi
sp("Barrel", CYL, (BX, BY, 42), (52, 52, 84), S["rust"])
sp("Barrel_RingA", CYL, (BX, BY, 25), (54, 54, 3), S["iron"])
sp("Barrel_RingB", CYL, (BX, BY, 62), (54, 54, 3), S["iron"])
for i, (fx, fy, fh) in enumerate(((0, 0, 34), (9, 5, 24), (-8, -6, 26), (5, -10, 20))):
    sp(f"Barrel_Flame_{i}", CONE, (BX + fx, BY + fy, 84 + fh / 2), (16, 16, fh), S["flame"])
sub_light("Light_Barrel", (BX, BY, 125), 1800.0, 520.0, (1.0, 0.5, 0.18), shadows=True)

# ---------------------------------------------------------------- FALCI MASASI = ENCHANT
# (Buyucu kadin modeli onaylaninca masanin arkasina gelecek)
WX, WY = 600.0, 35.0
en = interactable("Click_Enchant", enum_value(unreal.FortuneInteraction, "ENCHANT"), "Enchant", (WX, WY, 90), (45, 45, 45))
en.set_folder_path(SF)
sp("Witch_Table", CYL, (WX, WY, 74), (80, 80, 4), S["wood"])
sp("Witch_Cloth", CONE, (WX, WY, 38), (92, 92, 76), S["cloth"])
sp("Witch_BallStand", CYL, (WX, WY, 79), (14, 14, 6), S["gold"])
sp("Witch_Ball", SPH, (WX, WY, 92), (22, 22, 22), S["crystal"])
for i, (cx, cy) in enumerate(((-22.0, -24.0), (20.0, 26.0))):
    sp(f"Witch_Candle_{i}", CYL, (WX + cx, WY + cy, 83), (4, 4, 14), S["candle"])
    sp(f"Witch_CandleFlame_{i}", SPH, (WX + cx, WY + cy, 92), (1.6, 1.6, 3), S["flame"])
for i, cy in enumerate((-40.0, -5.0, 30.0, 65.0, 100.0)):
    sp(f"Witch_Curtain_{i}", CYL, (X1 - 40, WY - 20 + cy, 140), (22, 38, 280), S["curtain"])
sp("Witch_CurtainRod", CYL, (X1 - 40, WY + 20, 282), (3, 3, 200), S["gold"], rot=R(roll=90))
sub_light("Light_Witch", (WX, WY, 120), 900.0, 300.0, (0.7, 0.3, 1.0))

# ---------------------------------------------------------------- sis
try:
    fog = actors.spawn_actor_from_class(unreal.LocalFogVolume, V(MIDX, (YL + YR) / 2, 150), R())
    fog.set_actor_label("Subway_Fog")
    fog.set_folder_path(SF)
    fog.set_actor_scale3d(V(LEN / 200.0, (YR - YL) / 200.0, 2.0))
    fc = fog.get_component_by_class(unreal.LocalFogVolumeComponent)
    for k, v in (("radial_fog_extinction", 0.25), ("height_fog_extinction", 0.5)):
        try:
            fc.set_editor_property(k, v)
        except Exception:
            pass
except Exception as e:
    unreal.log_warning(f"Sis eklenemedi: {e}")

unreal.log("FlipForFortune: metro istasyonu kuruldu.")

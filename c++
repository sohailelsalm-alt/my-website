===== FILE: project.godot =====

config_version=5

[application]
config/name="Voxel Legends Mobile"
run/main_scene="res://main.tscn"

[display]
window/size/viewport_width=1280
window/size/viewport_height=720
window/handheld/orientation=1

[rendering]
renderer/rendering_method="mobile"
renderer/rendering_method.mobile="mobile"
textures/default_filters/use_nearest_mipmap_filter=false
environment/defaults/default_clear_color=Color(0.42, 0.68, 0.92, 1)

===== FILE: main.tscn =====

[gd_scene load_steps=2 format=3]

[ext_resource type="Script" path="res://scripts/main.gd" id="1"]

[node name="Main" type="Node3D"]
script = ExtResource("1")

===== FILE: scripts/main.gd =====

extends Node3D

const WorldScript = preload("res://scripts/voxel_world.gd")
const PlayerSceneScript = preload("res://scripts/player_controller.gd")
const NetworkScript = preload("res://scripts/network_manager.gd")

var world: Node3D
var player

func _ready() -> void:
    world = Node3D.new()
    world.name = "VoxelWorld"
    world.set_script(WorldScript)
    add_child(world)

    player = CharacterBody3D.new()
    player.name = "LocalPlayer"
    player.set_script(PlayerSceneScript)
    add_child(player)
    player.position = Vector3(8, 12, 8)

    var network = Node.new()
    network.name = "NetworkManager"
    network.set_script(NetworkScript)
    add_child(network)

    var sun := DirectionalLight3D.new()
    sun.rotation_degrees = Vector3(-55, -30, 0)
    sun.light_energy = 1.2
    add_child(sun)

    var env := WorldEnvironment.new()
    var environment := Environment.new()
    environment.background_mode = Environment.BG_SKY
    environment.sky = Sky.new()
    environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
    environment.ambient_light_color = Color(0.72, 0.8, 0.92)
    environment.ambient_light_energy = 0.7
    env.environment = environment
    add_child(env)

===== FILE: scripts/voxel_world.gd =====

extends Node3D
## Prototype voxel terrain. Rebuilds a small chunk as one mesh for a simple starter.
## Production worlds should use chunk meshing, greedy meshing, and asynchronous generation.

const SIZE_X := 16
const SIZE_Z := 16
const MAX_HEIGHT := 5
const BLOCK_SIZE := 1.0

var blocks: Dictionary = {} # Vector3i -> int (1 grass, 2 dirt, 3 stone)
var terrain_mesh: MeshInstance3D

const FACE_DATA := [
    {"n": Vector3i(1,0,0), "v": [Vector3(0.5,-0.5,-0.5), Vector3(0.5,-0.5,0.5), Vector3(0.5,0.5,0.5), Vector3(0.5,0.5,-0.5)]},
    {"n": Vector3i(-1,0,0), "v": [Vector3(-0.5,-0.5,0.5), Vector3(-0.5,-0.5,-0.5), Vector3(-0.5,0.5,-0.5), Vector3(-0.5,0.5,0.5)]},
    {"n": Vector3i(0,1,0), "v": [Vector3(-0.5,0.5,-0.5), Vector3(0.5,0.5,-0.5), Vector3(0.5,0.5,0.5), Vector3(-0.5,0.5,0.5)]},
    {"n": Vector3i(0,-1,0), "v": [Vector3(-0.5,-0.5,0.5), Vector3(0.5,-0.5,0.5), Vector3(0.5,-0.5,-0.5), Vector3(-0.5,-0.5,-0.5)]},
    {"n": Vector3i(0,0,1), "v": [Vector3(0.5,-0.5,0.5), Vector3(-0.5,-0.5,0.5), Vector3(-0.5,0.5,0.5), Vector3(0.5,0.5,0.5)]},
    {"n": Vector3i(0,0,-1), "v": [Vector3(-0.5,-0.5,-0.5), Vector3(0.5,-0.5,-0.5), Vector3(0.5,0.5,-0.5), Vector3(-0.5,0.5,-0.5)]}
]

func _ready() -> void:
    _generate_flat_test_world()
    _rebuild_mesh()

func _generate_flat_test_world() -> void:
    for x in range(SIZE_X):
        for z in range(SIZE_Z):
            for y in range(MAX_HEIGHT):
                var block_type := 1 if y == MAX_HEIGHT - 1 else (2 if y > 1 else 3)
                blocks[Vector3i(x, y, z)] = block_type

func has_block(pos: Vector3i) -> bool:
    return blocks.has(pos)

func remove_block(pos: Vector3i) -> void:
    if not blocks.has(pos):
        return
    blocks.erase(pos)
    _rebuild_mesh()

func place_block(pos: Vector3i, block_type: int = 1) -> void:
    if blocks.has(pos) or pos.y < 0 or pos.y > 64:
        return
    blocks[pos] = block_type
    _rebuild_mesh()

func _rebuild_mesh() -> void:
    if is_instance_valid(terrain_mesh):
        terrain_mesh.queue_free()

    var st := SurfaceTool.new()
    st.begin(Mesh.PRIMITIVE_TRIANGLES)

    for key in blocks.keys():
        var block_pos: Vector3i = key
        for face in FACE_DATA:
            var neighbor: Vector3i = block_pos + face["n"]
            if blocks.has(neighbor):
                continue
            var verts: Array = face["v"]
            var base_color := _block_color(int(blocks[block_pos]))
            var normal: Vector3 = Vector3(face["n"])
            st.set_color(base_color)
            st.set_normal(normal)
            st.add_vertex(Vector3(block_pos) + verts[0])
            st.set_color(base_color)
            st.set_normal(normal)
            st.add_vertex(Vector3(block_pos) + verts[1])
            st.set_color(base_color)
            st.set_normal(normal)
            st.add_vertex(Vector3(block_pos) + verts[2])
            st.set_color(base_color)
            st.set_normal(normal)
            st.add_vertex(Vector3(block_pos) + verts[0])
            st.set_color(base_color)
            st.set_normal(normal)
            st.add_vertex(Vector3(block_pos) + verts[2])
            st.set_color(base_color)
            st.set_normal(normal)
            st.add_vertex(Vector3(block_pos) + verts[3])

    st.generate_normals()
    var mesh := st.commit()
    terrain_mesh = MeshInstance3D.new()
    terrain_mesh.name = "TerrainMesh"
    terrain_mesh.mesh = mesh

    var material := StandardMaterial3D.new()
    material.vertex_color_use_as_albedo = true
    material.roughness = 1.0
    terrain_mesh.material_override = material
    add_child(terrain_mesh)

    var body := StaticBody3D.new()
    body.name = "TerrainCollision"
    var shape := CollisionShape3D.new()
    var concave := ConcavePolygonShape3D.new()
    concave.set_faces(mesh.get_faces())
    shape.shape = concave
    body.add_child(shape)
    add_child(body)

func _block_color(block_type: int) -> Color:
    match block_type:
        1: return Color(0.28, 0.62, 0.22) # grass
        2: return Color(0.42, 0.27, 0.15) # dirt
        _: return Color(0.42, 0.44, 0.47) # stone

===== FILE: scripts/player_controller.gd =====

extends CharacterBody3D
## Desktop test controls: WASD, mouse look, left click mine, right click place.
## Add a mobile touch UI before Android release.

const SPEED := 5.5
const JUMP_VELOCITY := 4.5
const REACH := 6.0
var gravity: float = ProjectSettings.get_setting("physics/3d/default_gravity")
var camera: Camera3D
var pitch := 0.0

func _ready() -> void:
    name = "LocalPlayer"
    var collision := CollisionShape3D.new()
    var capsule := CapsuleShape3D.new()
    capsule.height = 1.8
    capsule.radius = 0.35
    collision.shape = capsule
    collision.position.y = 0.9
    add_child(collision)

    camera = Camera3D.new()
    camera.position = Vector3(0, 1.55, 0)
    camera.current = true
    add_child(camera)
    Input.mouse_mode = Input.MOUSE_MODE_CAPTURED

func _unhandled_input(event: InputEvent) -> void:
    if event is InputEventMouseMotion and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
        rotate_y(-event.relative.x * 0.0025)
        pitch = clamp(pitch - event.relative.y * 0.0025, -1.45, 1.45)
        camera.rotation.x = pitch
    elif event is InputEventKey and event.pressed and event.keycode == KEY_ESCAPE:
        Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
    elif event is InputEventMouseButton and event.pressed:
        if event.button_index == MOUSE_BUTTON_LEFT:
            _edit_target(false)
        elif event.button_index == MOUSE_BUTTON_RIGHT:
            _edit_target(true)

func _physics_process(delta: float) -> void:
    if not is_on_floor():
        velocity.y -= gravity * delta
    elif Input.is_key_pressed(KEY_SPACE):
        velocity.y = JUMP_VELOCITY

    var input_2d := Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
    var direction := (transform.basis * Vector3(input_2d.x, 0, input_2d.y)).normalized()
    if direction:
        velocity.x = direction.x * SPEED
        velocity.z = direction.z * SPEED
    else:
        velocity.x = move_toward(velocity.x, 0, SPEED)
        velocity.z = move_toward(velocity.z, 0, SPEED)
    move_and_slide()

func _edit_target(placing: bool) -> void:
    var from := camera.global_position
    var to := from + (-camera.global_transform.basis.z * REACH)
    var query := PhysicsRayQueryParameters3D.create(from, to)
    query.exclude = [self]
    var hit := get_world_3d().direct_space_state.intersect_ray(query)
    if hit.is_empty():
        return

    var world_node := get_parent().get_node_or_null("VoxelWorld")
    if world_node == null:
        return

    var point: Vector3 = hit.position
    var normal: Vector3 = hit.normal
    var cell: Vector3i
    if placing:
        cell = Vector3i(floor(point.x + normal.x * 0.51), floor(point.y + normal.y * 0.51), floor(point.z + normal.z * 0.51))
        world_node.place_block(cell, 1)
    else:
        cell = Vector3i(floor(point.x - normal.x * 0.01), floor(point.y - normal.y * 0.01), floor(point.z - normal.z * 0.01))
        world_node.remove_block(cell)

===== FILE: scripts/network_manager.gd =====

extends Node
## Basic ENet connection scaffold. This establishes connections only;
## player replication and server-authoritative block edits are next implementation steps.

const DEFAULT_PORT := 24567
const MAX_CLIENTS := 8

func host_game(port: int = DEFAULT_PORT) -> Error:
    var peer := ENetMultiplayerPeer.new()
    var err := peer.create_server(port, MAX_CLIENTS)
    if err != OK:
        push_error("Could not start server: %s" % error_string(err))
        return err
    multiplayer.multiplayer_peer = peer
    print("Server listening on UDP port ", port)
    return OK

func join_game(address: String, port: int = DEFAULT_PORT) -> Error:
    var peer := ENetMultiplayerPeer.new()
    var err := peer.create_client(address, port)
    if err != OK:
        push_error("Could not connect: %s" % error_string(err))
        return err
    multiplayer.multiplayer_peer = peer
    print("Connecting to ", address, ":", port)
    return OK

func disconnect_game() -> void:
    multiplayer.multiplayer_peer = OfflineMultiplayerPeer.new()

===== FILE: README.md =====

# Voxel Legends Mobile — starter prototype

This is a small Godot 4 starter scaffold, not a finished or store-ready game.

## Included
- Small generated voxel test terrain (grass/dirt/stone colors).
- First-person controller for desktop testing.
- Mine/place blocks with left/right mouse buttons.
- ENet host/join connection scaffold.

## Requirements
- Godot Engine 4.x (stable release recommended).
- For Android export: Android SDK, JDK, and Godot export templates.
- Network export must have Android `INTERNET` permission enabled.

## Run
1. Extract the ZIP.
2. Open `project.godot` in Godot.
3. Press F6/F5 to run.
4. Desktop controls: WASD or arrow keys, mouse to look, Space to jump, left-click to mine, right-click to place.

## Important limitations
- The terrain is a tiny flat prototype and rebuilds one mesh after each edit.
- The ENet script only starts/joins a session; it does not yet replicate players or world edits.
- The player controller has no mobile touch UI yet.
- Before a public multiplayer launch, implement server-authoritative edits, player replication, authentication, rate limits, save/load, reconnection, moderation/reporting, and stress tests.
- This is not a guarantee of zero bugs. Test on actual devices and on a hosted server.

## Next milestones
1. Split terrain into chunks and rebuild only changed chunks.
2. Add procedural terrain, caves, trees, inventory and crafting.
3. Add mobile joystick/look controls and touch buttons.
4. Implement authoritative multiplayer for player transforms and block changes.
5. Add persistence, accounts, abuse prevention, privacy policy and store assets.
6. Profile on low/mid-range Android phones and fix crashes/performance issues.

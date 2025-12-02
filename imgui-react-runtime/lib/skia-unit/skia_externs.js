const _paint_create = $SHBuiltin.extern_c(
  {},
  function paint_create_cwrap(): c_ptr {
    throw 0;
  }
);

const _paint_set_color = $SHBuiltin.extern_c(
  {},
  function paint_set_color_cwrap(_paint: c_ptr, _color: c_int): void {
    throw 0;
  }
);

const _color_from_rgba = $SHBuiltin.extern_c(
  {},
  function color_from_rgba_cwrap(
    r: c_int,
    g: c_int,
    b: c_int,
    a: c_int
  ): c_ptr {
    throw 0;
  }
);

const _canvas_draw_paint = $SHBuiltin.extern_c(
  {},
  function canvas_draw_paint_cwrap(_paint: c_ptr): void {
    throw 0;
  }
);

const _paint_delete = $SHBuiltin.extern_c(
  {},
  function paint_delete_cwrap(_paint: c_ptr): void {
    throw 0;
  }
);

const _draw_rect = $SHBuiltin.extern_c(
  {},
  function draw_rect_cwrap(
    x: c_float,
    y: c_float,
    width: c_float,
    height: c_float,
    _paint: c_ptr
  ): void {
    throw 0;
  }
);

const _draw_round_rect = $SHBuiltin.extern_c(
  {},
  function draw_round_rect_cwrap(
    x: c_float,
    y: c_float,
    width: c_float,
    height: c_float,
    rx: c_float,
    ry: c_float,
    _paint: c_ptr
  ): void {
    throw 0;
  }
);

const _draw_simple_text = $SHBuiltin.extern_c(
  {},
  function draw_simple_text_cwrap(
    textPtr: c_ptr,
    x: c_float,
    y: c_float,
    _font: c_ptr,
    _paint: c_ptr
  ): void {
    throw 0;
  }
);

const _create_font_manager = $SHBuiltin.extern_c(
  {},
  function create_font_manager_cwrap(pathPtr: c_ptr): void {
    throw 0;
  }
);

const _create_font = $SHBuiltin.extern_c(
  {},
  function create_font_cwrap(familyNamePtr: c_ptr, size: c_float): c_ptr {
    throw 0;
  }
);

const _font_delete = $SHBuiltin.extern_c(
  {},
  function font_delete_cwrap(_font: c_ptr): void {
    throw 0;
  }
);

const _sizeof_SkPaint = 80;
const _sizeof_SkFont = 24;

// Yoga layout bindings
const yoga_node_new = $SHBuiltin.extern_c({}, function yoga_node_new(): c_ptr {
  throw 0;
});

const yoga_node_free = $SHBuiltin.extern_c(
  {},
  function yoga_node_free(_node: c_ptr): void {
    throw 0;
  }
);

const yoga_node_set_flex_direction = $SHBuiltin.extern_c(
  {},
  function yoga_node_set_flex_direction(_node: c_ptr, direction: c_int): void {
    throw 0;
  }
);

const yoga_node_set_width = $SHBuiltin.extern_c(
  {},
  function yoga_node_set_width(_node: c_ptr, width: c_float): void {
    throw 0;
  }
);

const yoga_node_set_height = $SHBuiltin.extern_c(
  {},
  function yoga_node_set_height(_node: c_ptr, height: c_float): void {
    throw 0;
  }
);

const yoga_node_set_flex_grow = $SHBuiltin.extern_c(
  {},
  function yoga_node_set_flex_grow(_node: c_ptr, grow: c_float): void {
    throw 0;
  }
);

const yoga_node_set_flex_basis = $SHBuiltin.extern_c(
  {},
  function yoga_node_set_flex_basis(_node: c_ptr, basis: c_float): void {
    throw 0;
  }
);

const yoga_node_set_padding = $SHBuiltin.extern_c(
  {},
  function yoga_node_set_padding(
    _node: c_ptr,
    edge: c_int,
    padding: c_float
  ): void {
    throw 0;
  }
);

const yoga_node_set_margin = $SHBuiltin.extern_c(
  {},
  function yoga_node_set_margin(
    _node: c_ptr,
    edge: c_int,
    margin: c_float
  ): void {
    throw 0;
  }
);

const yoga_node_set_gap = $SHBuiltin.extern_c(
  {},
  function yoga_node_set_gap(_node: c_ptr, gutter: c_int, gap: c_float): void {
    throw 0;
  }
);

const yoga_node_insert_child = $SHBuiltin.extern_c(
  {},
  function yoga_node_insert_child(
    _parent: c_ptr,
    _child: c_ptr,
    index: c_int
  ): void {
    throw 0;
  }
);

const yoga_node_remove_child = $SHBuiltin.extern_c(
  {},
  function yoga_node_remove_child(_parent: c_ptr, _child: c_ptr): void {
    throw 0;
  }
);

const yoga_node_calculate_layout = $SHBuiltin.extern_c(
  {},
  function yoga_node_calculate_layout(
    _root: c_ptr,
    width: c_float,
    height: c_float
  ): void {
    throw 0;
  }
);

const yoga_node_layout_get_left = $SHBuiltin.extern_c(
  {},
  function yoga_node_layout_get_left(_node: c_ptr): c_float {
    throw 0;
  }
);

const yoga_node_layout_get_top = $SHBuiltin.extern_c(
  {},
  function yoga_node_layout_get_top(_node: c_ptr): c_float {
    throw 0;
  }
);

const yoga_node_layout_get_width = $SHBuiltin.extern_c(
  {},
  function yoga_node_layout_get_width(_node: c_ptr): c_float {
    throw 0;
  }
);

const yoga_node_layout_get_height = $SHBuiltin.extern_c(
  {},
  function yoga_node_layout_get_height(_node: c_ptr): c_float {
    throw 0;
  }
);

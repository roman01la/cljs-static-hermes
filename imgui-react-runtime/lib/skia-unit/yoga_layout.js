// Yoga layout engine bindings

// Yoga enums
const YGFlexDirection = {
  Column: 0,
  Row: 1,
  ColumnReverse: 2,
  RowReverse: 3,
};

const YGEdge = {
  Left: 0,
  Top: 1,
  Right: 2,
  Bottom: 3,
  Start: 4,
  End: 5,
  Horizontal: 6,
  Vertical: 7,
  All: 8,
};

const YGJustify = {
  FlexStart: 0,
  Center: 1,
  FlexEnd: 2,
  SpaceBetween: 3,
  SpaceAround: 4,
  SpaceEvenly: 5,
};

const YGAlign = {
  Auto: 0,
  FlexStart: 1,
  Center: 2,
  FlexEnd: 3,
  Stretch: 4,
  Baseline: 5,
  SpaceBetween: 6,
  SpaceAround: 7,
};

const YGGutter = {
  Column: 0,
  Row: 1,
  All: 2,
};

function calculateLayout(root: YogaNode, width: number, height: number): void {
  yoga_node_calculate_layout(root.native, width, height);
}

class YogaNode {
  native: any;

  constructor(nativeNode: any) {
    this.native = nativeNode;
  }

  setFlexDirection(direction: number): void {
    yoga_node_set_flex_direction(this.native, direction);
  }

  setWidth(width: number): void {
    yoga_node_set_width(this.native, width);
  }

  setHeight(height: number): void {
    yoga_node_set_height(this.native, height);
  }

  setFlexGrow(grow: number): void {
    yoga_node_set_flex_grow(this.native, grow);
  }

  setPadding(edge: number, padding: number): void {
    yoga_node_set_padding(this.native, edge, padding);
  }

  setMargin(edge: number, margin: number): void {
    yoga_node_set_margin(this.native, edge, margin);
  }

  setGap(gutter: number, gap: number): void {
    yoga_node_set_gap(this.native, gutter, gap);
  }

  insertChild(child: YogaNode, index: number): void {
    yoga_node_insert_child(this.native, child.native, index);
  }

  removeChild(child: YogaNode): void {
    yoga_node_remove_child(this.native, child.native);
  }

  getLeft(): number {
    return yoga_node_layout_get_left(this.native);
  }

  getTop(): number {
    return yoga_node_layout_get_top(this.native);
  }

  getWidth(): number {
    return yoga_node_layout_get_width(this.native);
  }

  getHeight(): number {
    return yoga_node_layout_get_height(this.native);
  }

  getLayout(): any {
    return {
      left: this.getLeft(),
      top: this.getTop(),
      width: this.getWidth(),
      height: this.getHeight(),
    };
  }

  free(): void {
    yoga_node_free(this.native);
  }
}

// Helper to create new yoga node
function createYogaNode(): YogaNode {
  return new YogaNode(yoga_node_new());
}

// Apply flexbox styling props from React node to Yoga node
function applyFlexboxProps(yogaNode: YogaNode, props: any): void {
  if (!props) return;

  // Flex direction
  if (props.flexDirection !== undefined) {
    let direction;
    if (props.flexDirection === 'row') {
      direction = YGFlexDirection.Row;
    } else if (props.flexDirection === 'row-reverse') {
      direction = YGFlexDirection.RowReverse;
    } else if (props.flexDirection === 'column-reverse') {
      direction = YGFlexDirection.ColumnReverse;
    } else {
      direction = YGFlexDirection.Column; // default to column
    }
    yogaNode.setFlexDirection(direction);
  }

  // Dimensions
  if (props.width !== undefined && props.width > 0) {
    yogaNode.setWidth(props.width);
  }
  if (props.height !== undefined && props.height > 0) {
    yogaNode.setHeight(props.height);
  }

  // Flex properties
  if (props.flex !== undefined) {
    yogaNode.setFlexGrow(props.flex);
  }
  if (props.flexGrow !== undefined) {
    yogaNode.setFlexGrow(props.flexGrow);
  }

  // Padding
  if (props.padding !== undefined) {
    yogaNode.setPadding(YGEdge.All, props.padding);
  }
  if (props.paddingLeft !== undefined) {
    yogaNode.setPadding(YGEdge.Left, props.paddingLeft);
  }
  if (props.paddingRight !== undefined) {
    yogaNode.setPadding(YGEdge.Right, props.paddingRight);
  }
  if (props.paddingTop !== undefined) {
    yogaNode.setPadding(YGEdge.Top, props.paddingTop);
  }
  if (props.paddingBottom !== undefined) {
    yogaNode.setPadding(YGEdge.Bottom, props.paddingBottom);
  }

  // Margin
  if (props.margin !== undefined) {
    yogaNode.setMargin(YGEdge.All, props.margin);
  }
  if (props.marginLeft !== undefined) {
    yogaNode.setMargin(YGEdge.Left, props.marginLeft);
  }
  if (props.marginRight !== undefined) {
    yogaNode.setMargin(YGEdge.Right, props.marginRight);
  }
  if (props.marginTop !== undefined) {
    yogaNode.setMargin(YGEdge.Top, props.marginTop);
  }
  if (props.marginBottom !== undefined) {
    yogaNode.setMargin(YGEdge.Bottom, props.marginBottom);
  }

  // Gap
  if (props.gap !== undefined) {
    yogaNode.setGap(YGGutter.All, props.gap);
  }
  if (props.columnGap !== undefined) {
    yogaNode.setGap(YGGutter.Column, props.columnGap);
  }
  if (props.rowGap !== undefined) {
    yogaNode.setGap(YGGutter.Row, props.rowGap);
  }
}

globalThis.yoga = {
  FlexDirection: YGFlexDirection,
  Justify: YGJustify,
  Align: YGAlign,
  Gutter: YGGutter,
  Edge: YGEdge,
  createNode: createYogaNode,
  YogaNode: YogaNode,
};

// Attach yoga node to React node tree
function attachYogaNode(reactNode: any): void {
  if (!reactNode.yoga) {
    reactNode.yoga = createYogaNode();
  }
}

// Apply Yoga layout to React node tree
function applyYogaLayout(node: any): void {
  // Create fresh yoga node for each frame to avoid ownership issues
  node.yoga = createYogaNode();

  // Apply flexbox styling props to this node's yoga node
  if (node.props) {
    applyFlexboxProps(node.yoga, node.props);
  }

  // Process children
  if (node.children && node.children.length > 0) {
    // First recursively apply layout to all children
    for (let i = 0; i < node.children.length; i++) {
      applyYogaLayout(node.children[i]);
    }

    // Then insert children into yoga tree
    for (let i = 0; i < node.children.length; i++) {
      if (node.children[i].yoga) {
        node.yoga.insertChild(node.children[i].yoga, i);
      }
    }
  }

  // Calculate layout for this subtree
  calculateLayout(node.yoga, node.props.width || 0, node.props.height || 0);

  // Store computed layout in _layout with parent offset (0, 0 for root)
  const layout = node.yoga.getLayout();
  node._layout = {
    x: layout.left,
    y: layout.top,
    width: layout.width,
    height: layout.height,
  };

  // Apply layout to children
  if (node.children && node.children.length > 0) {
    for (let i = 0; i < node.children.length; i++) {
      updateNodeFromYoga(node.children[i], node._layout.x, node._layout.y);
    }
  }
}

function updateNodeFromYoga(
  node: any,
  parentX: number = 0,
  parentY: number = 0
): void {
  if (node.yoga) {
    const layout = node.yoga.getLayout();
    // Position child relative to parent's position
    node._layout = {
      x: parentX + layout.left,
      y: parentY + layout.top,
      width: layout.width,
      height: layout.height,
    };
  }

  if (node.children && node.children.length > 0) {
    for (let i = 0; i < node.children.length; i++) {
      updateNodeFromYoga(node.children[i], node._layout.x, node._layout.y);
    }
  }
}

// Clean up yoga nodes recursively
function freeYogaNodes(node: any): void {
  if (!node) return;

  // Free children first
  if (node.children && node.children.length > 0) {
    for (let i = 0; i < node.children.length; i++) {
      freeYogaNodes(node.children[i]);
    }
  }

  // Free this node's yoga instance
  if (node.yoga) {
    node.yoga.free();
    node.yoga = null;
  }
}

globalThis.yogaLayout = {
  attachYogaNode: attachYogaNode,
  applyYogaLayout: applyYogaLayout,
  updateNodeFromYoga: updateNodeFromYoga,
  freeYogaNodes: freeYogaNodes,
};

function renderRect(node: any): void {
  const paint = allocTmp(_sizeof_SkPaint);

  _paint_set_color(paint, node.props.backgroundColor);

  const x = node.props.x;
  const y = node.props.y;
  const width = node.props.width;
  const height = node.props.height;

  if (node.props.borderRadius) {
    const r = node.props.borderRadius;
    _draw_round_rect(x, y, width, height, r, r, paint);
  } else {
    _draw_rect(x, y, width, height, paint);
  }
}

function renderNode(node: any): void {
  if (!node) return;

  if (node.text !== undefined) {
    // _igText(tmpUtf8(node.text));
    return;
  }

  switch (node.type) {
    case 'rect':
      renderRect(node);
      break;
  }

  for (let i = 0; i < node.children.length; i++) {
    renderNode(node.children[i]);
  }
}

globalThis.skiaUnit = {
  renderTree: function (): void {
    const reactApp = globalThis.reactApp;
    for (let i = 0; i < reactApp.rootChildren.length; i++) {
      renderNode(reactApp.rootChildren[i]);
    }
  },
};

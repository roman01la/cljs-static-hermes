function getLayout(node: any): [number, number, number, number] {
  // Use computed layout if available, otherwise fall back to props
  const layout = node._layout || node.props;
  const x = layout.x ?? node.props.x ?? 0;
  const y = layout.y ?? node.props.y ?? 0;
  const width = layout.width ?? node.props.width ?? 0;
  const height = layout.height ?? node.props.height ?? 0;
  return [x, y, width, height];
}

function renderRect(node: any): void {
  const paint = allocTmp(_sizeof_SkPaint);

  _paint_set_color(paint, node.props.backgroundColor);

  const [x, y, width, height] = getLayout(node);

  if (node.props.borderRadius) {
    const r = node.props.borderRadius;
    _draw_round_rect(x, y, width, height, r, r, paint);
  } else {
    _draw_rect(x, y, width, height, paint);
  }
}

_create_font_manager(
  tmpUtf8(
    '/Users/romanliutikov/projects/cljs-static-hermes/imgui-react-runtime/external/skia/skia/resources/fonts/'
  )
);

const fontsCache: any = {};

function renderText(node: any): void {
  const paint = allocTmp(_sizeof_SkPaint);
  _paint_set_color(paint, node.props.color);
  const key = node.props.fontFamily + '_' + node.props.fontSize;
  fontsCache[key] =
    fontsCache[key] ||
    _create_font(
      tmpUtf8(node.props.fontFamily),
      node.props.fontSize * globalThis.devicePixelRatio
    );
  const font = fontsCache[key];
  const [x, y, width, height] = getLayout(node);
  _draw_simple_text(tmpUtf8(node.props.children), x, y, font, paint);
}

function renderNode(node: any): void {
  if (!node) return;

  switch (node.type) {
    case 'rect':
      renderRect(node);
      break;
    case 'text':
      renderText(node);
      return;
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

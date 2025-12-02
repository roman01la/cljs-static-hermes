// Copyright (c) Tzvetan Mikov and contributors
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text

// Main entry point for Skia unit
// Provides initialization and rendering for Skia graphics

globalThis.on_init = function on_init(): void {
  globalThis.reactApp.render();
};

globalThis.on_frame = function on_frame(
  width: number,
  height: number,
  curTime: number
): void {
  // Flush temporary allocations from previous frame
  flushAllocTmp();

  // Compute Yoga layout before rendering
  const rootChildren = globalThis.reactApp.rootChildren;
  for (let i = 0; i < rootChildren.length; i++) {
    const root = rootChildren[i];
    if (root.type === 'root') {
      root.props = {};
      root.props.width = width;
      root.props.height = height;
      root.props.x = 0;
      root.props.y = 0;
    }
    globalThis.yogaLayout.attachYogaNode(root);
    globalThis.yogaLayout.applyYogaLayout(root);
  }

  globalThis.skiaUnit.renderTree();
};

globalThis.on_exit = function on_exit(): void {
  // Free all yoga nodes when exiting
  const rootChildren = globalThis.reactApp.rootChildren;
  for (let i = 0; i < rootChildren.length; i++) {
    globalThis.yogaLayout.freeYogaNodes(rootChildren[i]);
  }
};

const CAPTURE_PHASE = 1;
const AT_TARGET = 2;
const BUBBLE_PHASE = 3;

let sPreviousMouseTarget: any = null;

function findNodeAtPoint(node: any, x: number, y: number): any {
  // Check if node contains the point
  if (!node || node.text) {
    return null;
  }

  // Use computed layout if available, otherwise fall back to props
  const layout = node._layout || node.props;
  const left: number = layout.x ?? node.props.x ?? 0;
  const top: number = layout.y ?? node.props.y ?? 0;
  const width: number = layout.width ?? node.props.width ?? 0;
  const height: number = layout.height ?? node.props.height ?? 0;

  if (x < left || x >= left + width || y < top || y >= top + height) {
    return null;
  }

  // Check children first (depth-first)
  if (node.children) {
    for (let i = node.children.length - 1; i >= 0; i--) {
      const found = findNodeAtPoint(node.children[i], x, y);
      if (found) {
        return found;
      }
    }
  }

  return node;
}

function buildEventPath(node: any): any[] {
  const path = [];
  let current = node;
  while (current) {
    path.unshift(current);
    current = current.parent;
  }
  return path;
}

function isAncestor(ancestor: any, node: any): boolean {
  let current = node;
  while (current) {
    if (current === ancestor) {
      return true;
    }
    current = current.parent;
  }
  return false;
}

function dispatchMouseEnterLeave(newTarget: any): void {
  if (newTarget === sPreviousMouseTarget) {
    return;
  }

  // Build paths for comparison
  const prevPath: any = sPreviousMouseTarget
    ? buildEventPath(sPreviousMouseTarget)
    : [];
  const newPath: any = newTarget ? buildEventPath(newTarget) : [];

  // Find lowest common ancestor
  let commonAncestorIdx = -1;
  for (let i = 0; i < prevPath.length && i < newPath.length; i++) {
    if (prevPath[i] === newPath[i]) {
      commonAncestorIdx = i;
    } else {
      break;
    }
  }

  // Dispatch mouseleave to previous target and its ancestors (up to common ancestor)
  if (sPreviousMouseTarget) {
    for (let i = prevPath.length - 1; i > commonAncestorIdx; i--) {
      const node = prevPath[i];
      if (node.props && node.props.onMouseLeave) {
        const leaveEvent = {
          type: 'onMouseLeave',
          target: newTarget,
          currentTarget: node,
          eventPhase: AT_TARGET,
          bubbles: false,
          cancelable: false,
          defaultPrevented: false,
          preventDefault: function (): void {},
          stopPropagation: function (): void {},
          stopImmediatePropagation: function (): void {},
        };
        node.props.onMouseLeave.call(node, leaveEvent);
      }
    }
  }

  // Dispatch mouseenter to new target and its ancestors (from common ancestor down to target)
  if (newTarget) {
    for (let i = commonAncestorIdx + 1; i < newPath.length; i++) {
      const node = newPath[i];
      if (node.props && node.props.onMouseEnter) {
        const enterEvent = {
          type: 'onMouseEnter',
          target: newTarget,
          currentTarget: node,
          eventPhase: AT_TARGET,
          bubbles: false,
          cancelable: false,
          defaultPrevented: false,
          preventDefault: function (): void {},
          stopPropagation: function (): void {},
          stopImmediatePropagation: function (): void {},
        };
        node.props.onMouseEnter.call(node, enterEvent);
      }
    }
  }

  sPreviousMouseTarget = newTarget;
}

function propagateEvent(event: any): void {
  const reactApp = globalThis.reactApp;
  let target: any = null;

  // Find the target node
  for (let i = 0; i < reactApp.rootChildren.length; i++) {
    const found = findNodeAtPoint(reactApp.rootChildren[i], event.x, event.y);
    if (found) {
      target = found;
      break;
    }
  }

  if (target === null) {
    return;
  }

  event.target = target;
  event.bubbles = event.bubbles !== false; // default true
  event.cancelable = event.cancelable !== false; // default true
  event.defaultPrevented = false;

  let shouldBubble = true;
  let shouldStop = false;
  let shouldStopImmediate = false;

  // Setup event methods
  event.preventDefault = function (): void {
    if (event.cancelable) {
      event.defaultPrevented = true;
    }
  };

  event.stopPropagation = function (): void {
    shouldBubble = false;
  };

  event.stopImmediatePropagation = function (): void {
    shouldBubble = false;
    shouldStopImmediate = true;
  };

  // Build the event path
  const path = buildEventPath(target);

  // CAPTURE PHASE: Walk down from root to target
  event.eventPhase = CAPTURE_PHASE;
  for (let i = 0; i < path.length - 1; i++) {
    const node = path[i];
    event.currentTarget = node;

    if (node.props[event.type]) {
      const listener = node.props[event.type];
      if (listener) {
        listener.call(node, event);
        if (shouldStopImmediate) {
          shouldStopImmediate = false;
          shouldBubble = false;
          return;
        }
      }
      if (!shouldBubble) {
        return;
      }
    }
  }

  // AT TARGET PHASE
  event.eventPhase = AT_TARGET;
  event.currentTarget = target;

  if (target.props[event.type]) {
    const listener = target.props[event.type];
    if (listener) {
      listener.call(target, event);
    }
    if (shouldStopImmediate) {
      shouldStopImmediate = false;
      shouldBubble = false;
      return;
    }
  }

  if (!shouldBubble) {
    return;
  }

  // BUBBLE PHASE: Walk up from target to root
  if (event.bubbles) {
    event.eventPhase = BUBBLE_PHASE;
    for (let i = path.length - 2; i >= 0; i--) {
      const node = path[i];
      event.currentTarget = node;

      if (node.props[event.type]) {
        const listener = node.props[event.type];
        if (listener) {
          listener.call(node, event);
        }
        if (shouldStopImmediate) {
          shouldStopImmediate = false;
          shouldBubble = false;
          break;
        }
        if (!shouldBubble) {
          break;
        }
      }
    }
  }
}

function findTarget(): any {
  const reactApp = globalThis.reactApp;
  let currentTarget: any = null;
  for (let i = 0; i < reactApp.rootChildren.length; i++) {
    const found = findNodeAtPoint(reactApp.rootChildren[i], mx, my);
    if (found) {
      currentTarget = found;
      break;
    }
  }
  return currentTarget;
}

let mx: number = 0;
let my: number = 0;
let sMouseDownTarget: any = null;

globalThis.on_event = function on_event(
  type: string,
  key_code: number,
  modifiers: number
): void {
  if (type === 'mousemove') {
    const x = key_code;
    const y = modifiers;

    mx = x;
    my = y;

    // Find target node for mouseenter/mouseleave
    const target: any = findTarget();

    // Dispatch mouseenter/mouseleave events
    dispatchMouseEnterLeave(target);

    // Dispatch mousemove event
    propagateEvent({ type: 'onMouseMove', x, y });
  } else if (type === 'mousebutton') {
    const button = key_code;
    const action = modifiers;

    if (button === 0 && action === 1) {
      // Left button down
      propagateEvent({ type: 'onMouseDown', x: mx, y: my, button });
      // Store the target for click detection
      sMouseDownTarget = findTarget();
    } else if (button === 0 && action === 0) {
      // Left button up
      propagateEvent({ type: 'onMouseUp', x: mx, y: my, button });

      // Dispatch click if mouseup target is same as mousedown target
      if (sMouseDownTarget) {
        const currentTarget: any = findTarget();

        // Check if we're on the same target or a descendant/ancestor
        if (
          currentTarget === sMouseDownTarget ||
          isAncestor(sMouseDownTarget, currentTarget) ||
          isAncestor(currentTarget, sMouseDownTarget)
        ) {
          propagateEvent({ type: 'onClick', x: mx, y: my, button });
        }
      }

      sMouseDownTarget = null;
    }
  }
};

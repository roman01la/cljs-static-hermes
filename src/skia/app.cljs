(ns skia.app
  (:require [uix.core :as uix :refer [$ defui]]
            ["react-imgui-reconciler/reconciler.js" :as rir]))

(defui app []
  (let [[hover? set-hover] (uix/use-state false)
        [active? set-active] (uix/use-state false)]
    ($ :rect {:x 100 :y 200 :width 300 :height 500
              :background-color (if active? 0xFF00FF00 0xFF0000FF)
              :on-click #(set-active not)}
      ($ :rect {:x 200 :y 230 :width 100 :height 100
                :background-color (if hover? 0xFF00FF00 0xFFFF0000)
                :border-radius 16
                :on-mouse-enter #(set-hover true)
                :on-mouse-leave #(set-hover false)}))))

(defonce reload-fn (atom nil))

(defn register [{:keys [title width height component]}]
  ;; create react root
  (let [root (rir/createRoot)
        render-fn* #(let [component (if (var? component) @component component)]
                      (rir/render ($ component) root))]
    ;; Configure window (optional - defaults are provided)
    (set! (.-sappConfig js/globalThis)
          #js {:title title
               :width width
               :height height})
    (reset! reload-fn render-fn*)
    ;; register root rendering
    (set! (.-reactApp js/globalThis)
          #js {:rootChildren #js []
               :render render-fn*})))

(defn init []
  (register
    {:title "ClojureScript + UIx + React + Skia Showcase"
     :width 1024
     :height 768
     :component #'app}))

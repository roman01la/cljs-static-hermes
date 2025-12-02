(ns skia.app
  (:require [uix.core :as uix :refer [$ defui]]
            ["react-imgui-reconciler/reconciler.js" :as rir]))

(defui box [{:keys [flex-direction color children hover-enabled?] :or {hover-enabled? true flex-direction :row}}]
  (let [[hover? set-hover] (uix/use-state false)]
    ($ :rect {:flex 1
              :flex-direction flex-direction
              :background-color (if hover? 0xFF00FF00 (or color 0xFF00EEFF))
              :border-radius 16
              :padding 16
              :gap 16
              :on-mouse-enter (when hover-enabled? #(set-hover true))
              :on-mouse-leave (when hover-enabled? #(set-hover false))}
       children)))

(defui app []
  (let [[active? set-active] (uix/use-state false)]
    ($ :root
      ($ :rect {:flex 1
                :padding 32
                :gap 44
                :background-color (if active? 0xFF00FF00 0xFF0000FF)
                :on-click #(set-active not)}
        ($ box {:hover-enabled? false}
           ($ box {:color 0xFFAAFF00})
           ($ box {:color 0xFFEEFF00})
           ($ box {:color 0xFFFF00FF}))
        ($ box {:flex-direction :column}
           ($ :text {:flex 1
                     :font-family "Roboto"
                     :font-size 24
                     :color 0xff000000}
             "What is Lorem Ipsum?")
           ($ :text {:flex 1
                     :font-family "Roboto"
                     :font-size 16
                     :color 0xff000000}
              "Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum."))))))

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
    {:title "ClojureScript + UIx + React + Skia + Yoga Showcase"
     :width 1024
     :height 768
     :component #'app}))

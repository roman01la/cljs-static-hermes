(ns hermes.app
  (:require [uix.core :as uix :refer [$ defui]]
            [hermes.client]))

(defui background []
  ($ :<>
    ($ :rect {:x 10 :y 10 :width 150 :height 100 :color "#3030A0C0" :filled true})
    ($ :rect {:x 200 :y 400 :width 180 :height 80 :color "#A03030C0" :filled true})
    ($ :circle {:x 700 :y 500 :radius 60 :color "#30A030C0" :filled true :segments 32})
    ($ :circle {:x 900 :y 100 :radius 40 :color "#A0A030C0" :filled true :segments 24})))

(defui status-bar [{:keys [counter1 counter2]}]
  ($ :<>
    ($ :rect {:x 0
              :y 0
              :width 1200
              :height 25
              :color "#00000080"
              :filled true})
    ($ :text {:color "#00FF00"}
       "ClojureScript + UIx + React + ImGui Showcase")
    ($ :sameline)
    ($ :text {:color "#FFFF00"}
       "  |  Counters: " counter1 "/" counter2)
    ($ :sameline)
    ($ :text {:color "#00FFFF"}
       "  |  Total clicks: " (+ counter1 counter2))))

(defui bouncing-ball []
  (let [content-width 400
        content-height 300
        border-thickness 4
        ball-radius 20
        [bx set-bx] (uix/use-state 200)
        [by set-by] (uix/use-state 150)
        speed 5.0
        angle (* (Math/random) 2 Math/PI)
        [vx set-vx] (uix/use-state (* speed (Math/cos angle)))
        [vy set-vy] (uix/use-state (* speed (Math/sin angle)))]

    (uix/use-effect
      (fn []
        (let [id (atom nil)]
          (reset! id
            (js/requestAnimationFrame
              (fn render []
                (let [update-ball (fn [b v set-v side]
                                    (let [c (+ b v)]
                                      (if (or (<= (- c ball-radius) border-thickness)
                                              (>= (+ c ball-radius) (- side border-thickness)))
                                        (do (set-v #(- %))
                                            (if (<= (- c ball-radius) border-thickness)
                                              (+ ball-radius border-thickness)
                                              (- side ball-radius border-thickness)))
                                        c)))]
                  (set-bx #(update-ball % vx set-vx content-width))
                  (set-by #(update-ball % vy set-vy content-height))
                  (reset! id (js/requestAnimationFrame render))))))
          #(js/cancelAnimationFrame @id)))
      [ball-radius border-thickness content-width content-height vx vy])

    ($ :window {:title "Bouncing Ball"
                :default-x 600
                :default-y 350
                :flags 64} ;; ImGuiWindowFlags_AlwaysAutoResize
      ($ :child {:width content-width
                 :height content-height
                 :no-padding true
                 :no-scrollbar true}
        ;; White borders - top, left, bottom, right
        ($ :rect {:x 0 :y 0 :width content-width :height border-thickness :color "#FFFFFF" :filled true})
        ($ :rect {:x 0 :y 0 :width border-thickness :height content-height :color "#FFFFFF" :filled true})
        ($ :rect {:x 0 :y (- content-height border-thickness) :width content-width :height border-thickness :color "#FFFFFF" :filled true})
        ($ :rect {:x (- content-width border-thickness) :y 0 :width border-thickness :height content-height :color "#FFFFFF" :filled true})
        ;; Green bouncing ball
        ($ :circle {:x bx :y by :radius ball-radius :color "#00FF00" :filled true})))))

(def cities
  ["Tokyo", "Delhi", "Shanghai", "Sao Paulo", "Mumbai", "Mexico City",
   "Beijing", "Osaka", "Cairo", "New York", "Dhaka", "Karachi",
   "Buenos Aires", "Kolkata", "Istanbul", "Rio de Janeiro", "Manila",])

(def rows (count cities))
(def cols 8)

(defn value->color [value]
  (cond
    (< value 33) "#FF00ff"
    (< value 66) "#0000ff"
    :else "#FFFFFF"))

(defui stock-table []
  (let [[data set-data] (uix/use-state #(for [_ (range rows)]
                                          (for [_ (range cols)]
                                            (* 1e2 (Math/random)))))]
    (uix/use-effect
      (fn []
        (let [id (js/setInterval
                   (fn []
                     (set-data
                       (fn [prev-data]
                         (for [i (range rows)]
                           (for [j (range cols)]
                             (min 100 (max 0 (+ (nth (nth prev-data i) j)
                                                (* 2 (- (Math/random) 0.5))))))))))
                   1000)]
          #(js/clearInterval id)))
      [])

    ($ :window {:title "Cities Stock Prices"
                :default-x 100
                :default-y 150
                :default-width 600}
      ($ :table {:id :stockTable
                 :columns (inc cols)}
        ($ :tablecolumn {:label "City" :flags 16 :width 0}
         (for [col (range cols)]
           ($ :tablecolumn {:key col :label (str "Col " (inc col)) :flags 8 :width 0})))
        ($ :tableheader)
        (map-indexed
          (fn [idx row]
            ($ :tablerow {:key idx}
              ($ :tablecell {:index 0}
                ($ :text (nth cities (mod idx (count cities)))))
              (map-indexed
                (fn [idx v]
                  ($ :tablecell {:key idx :index (inc idx)}
                    ($ :text {:color (value->color v)}
                      (.toFixed v 2))))
                row)))
          data)))))

(defui controlled-window []
  #_(throw (js/Error. "hello"))
  (let [[{:keys [x y width height]} set-window] (uix/use-state {:x 300 :y 300 :width 350 :height 250})
        snap-to-origin #(set-window merge {:x 20 :y 20})
        snap-to-center #(set-window merge {:x 400 :y 300})
        make-wide #(set-window merge {:width 600 :height 250})
        make-tall #(set-window merge {:width 350 :height 400})]
    ($ :window {:title "Controlled Window Demo"
                :x x :y y :width width :height height
                :on-window-state #(set-window (zipmap [:x :y :width :height] %&))}
      ($ :text {:color "#FFAA00"}
         "This window is CONTROLLED by React state")
      ($ :text "Try moving or resizing it - state updates automatically!")
      ($ :separator)
      ($ :text {:color "#00FF00"} "Programmatic Control:")
      ($ :button {:on-click snap-to-origin}
         "Snap to Origin (20, 20)")
      ($ :button {:on-click snap-to-center}
         "Snap to Center (400, 300)")
      ($ :separator)
      ($ :button {:on-click make-wide}
         "Make Wide (600x250)")
      ($ :sameline)
      ($ :button {:on-click make-tall}
         "Make Tall (350x400)")
      ($ :separator)
      ($ :collapsingheader {:title "How This Works"}
         ($ :text {:wrapped true}
            "This window uses x, y, width, and height props (not defaultX/defaultY). These props are enforced every frame using ImGuiCond_Always.")
         ($ :text {:wrapped true}
            "When you move or resize the window, onWindowState fires with new values. We update React state, which updates the props, completing the cycle.")
         ($ :text {:wrapped true}
            "The buttons demonstrate programmatic control: just update state!")))))

(defui app []
  (let [[counter1 set-counter-1] (uix/use-state 0)
        [counter2 set-counter-2] (uix/use-state 0)]
    ($ :<>
      ;; Background decorations - rendered first, behind windows
      ($ background)
       ;; Status bar at the top
      ($ status-bar {:counter1 counter1 :counter2 counter2})
      ;; All the existing windows
      ($ bouncing-ball)
      ($ stock-table)
      ($ controlled-window)
      ($ :window {:title "Hello from React!"
                  :default-x 20
                  :default-y 40}
         ($ :text "This is a React component rendering to ImGui")
         ($ :text "React's reconciler is working perfectly!")
         ($ :separator)
         ($ :button {:on-click #(set-counter-1 inc)}
            "Click me!")
         ($ :sameline)
         ($ :text
            "Button clicked " counter1 " times"))
      ($ :window {:title "Component Playground"
                  :default-x 650
                  :default-y 40}
         ($ :text {:color "#00FFFF"}
            "Welcome to the ClojureScript + UIx + React + ImGui demo!")
         ($ :separator)
         ($ :group
            ($ :text {:color "#FFFF00"} "Counter Demo:")
            ($ :button {:on-click #(set-counter-2 inc)}
               "Increment")
            ($ :sameline)
            ($ :button {:on-click #(set-counter-2 dec)}
               "Decrement")
            ($ :sameline)
            ($ :button {:on-click #(set-counter-2 0)}
               "Reset")
            ($ :text {:color (if (zero? counter2) "#888888" "#FFFFFF")}
               "Current value: " counter2))
         ($ :separator)
         ($ :group
            ($ :text {:color "#FFFF00"} "Quick Math:")
            ($ :indent
               ($ :text {:color "#00FF00"} "Counter x 2 = " (* 2 counter2))
               ($ :text {:color "#00FF00"} "Counter squared = " (* counter2 counter2))
               ($ :text {:color "#00FFFF"} "Counter is " (if (even? counter2) "EVEN" "ODD"))))
         ($ :separator)
         ($ :group
           ($ :text {:color "#FFFF00"} "Status Indicators:")
           ($ :indent
              ($ :text {:color (if (> counter2 10) "#FF4444" "#4444FF")}
                 (if (> counter2 10) "[HOT] Counter is high!" "[COOL] Counter is low"))
              ($ :text {:color (if (neg? counter2) "#FFAA00" "#00FF00")}
                 (if (neg? counter2) "[WARN] Negative territory!" "[OK] Positive vibes"))))
         ($ :separator)
         ($ :collapsingheader {:title "Architecture Info"}
            ($ :text "React 19.2.0 with custom reconciler")
            ($ :text "Static Hermes (typed + untyped units)")
            ($ :text "Zero-overhead FFI to DearImGui")
            ($ :text "Event loop with setTimeout/Promises")
            ($ :text {:color "#FF00FF"} "Root component for fullscreen canvas!"))
         ($ :separator)
         ($ :text "Quick Actions:")
         ($ :button {:on-click #(set-counter-2 (Math/floor (* 1e2 (Math/random))))}
            "Random (0-99)")
         ($ :sameline)
         ($ :button {:on-click #(set-counter-2 (+ counter2 10))}
            "+10")
         ($ :button {:on-click #(set-counter-2 (- counter2 10))}
            "-10"))
      ;; Footer info bar
      ($ :rect {:x 0 :y 575 :width 1200 :height 25 :color "#00000080" :filled true})
      ($ :text {:color "#888888"} "Root component demo - Background shapes and overlay elements render behind/above all windows"))))

(defn init []
  (hermes.client/register
    {:title "ClojureScript + UIx + React + ImGui Showcase" :width 1024 :height 768}
    #($ app)))
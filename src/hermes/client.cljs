(ns hermes.client
  (:require [clojure.string :as str]
            [uix.core :as uix :refer [$ defui]]
            ["react-imgui-reconciler/reconciler.js" :as rir]))

(defonce reload-fn (atom nil))

(defn ^:dev/after-load reload []
  (@reload-fn))

(defui compiler-warnings-hud [{:keys [warnings]}]
  ($ :window {:title "Build warnings"
              :default-x 20
              :default-y 40}
    (for [{:keys [warnings]} warnings
          {:keys [resource-name msg file line column source-excerpt] :as warning} warnings
          :let [{:keys [start-idx before after] l :line} source-excerpt]]
      ($ :group {:key (str file line column)}
        ($ :text {:color "#ffff00"}
          (str resource-name ":" line ":" column "\n"
               msg))
        ($ :separator)
        ($ :text {:color "#ffff00"}
          (str (str/join "\n" before)
               "\n" l "\n"
               (.padStart "" column ".") "^"
               "\n"
               (str/join "\n" after)))))))

(defui compiler-errors-hud [{:keys [errors]}]
  ($ :window {:title "Build error"
              :default-x 20
              :default-y 40}
    ($ :text {:color "#ff0000"}
       (first errors))))

(defui hud []
  (let [{:keys [warnings errors]} (uix/use-atom (if (exists? js/hermes.repl.client.hud) js/hermes.repl.client.hud (atom nil)))]
    ($ :<>
      (when (seq warnings)
        ($ compiler-warnings-hud {:warnings warnings}))
      (when (seq errors)
        ($ compiler-errors-hud {:errors errors})))))

(defui runtime-error-hud [{:keys [error set-state]}]
  ($ :window {:title "Error"
              :default-x 150
              :default-y 150
              :default-width 500
              :default-height 300}
     ($ :text {:color "#FF0000"}
        (.-stack error))
     ($ :button {:on-click (fn []
                             (set-state {})
                             (reload))}
        "Restart")))

(def error-boundary
  (uix/create-error-boundary
    {:derive-error-state (fn [error] {:error error})}
    (fn [[{:keys [error]} set-state] {:keys [children]}]
      ($ :root
        (if error
          ($ runtime-error-hud {:error error :set-state set-state})
          children)
        ($ hud)))))

(defn register [{:keys [title width height]} render-root]
  ;; create react root
  (let [root (rir/createRoot)
        render-fn* #(rir/render ($ error-boundary (render-root)) root)]
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
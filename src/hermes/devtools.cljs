(ns hermes.devtools)

(set! (.-self js/globalThis) js/globalThis)
(set! (.-window js/globalThis) js/globalThis)

(def rdc (js/require "react-devtools-core/dist/backend"))

(.initialize rdc)

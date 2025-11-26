(ns hermes.persistent-vector-example
  "Example program demonstrating native PersistentVector with ClojureScript protocols.

   This example shows how the native C++ persistent vector can be used seamlessly
   with standard ClojureScript functions through protocol implementations."
  (:require [hermes.persistent-vector :as pv]))

(defn print-section [title]
  (println)
  (println (str "=== " title " ===")))

(defn run-examples []
  (println "")
  (println "╔════════════════════════════════════════════════════════════════╗")
  (println "║  Native PersistentVector with ClojureScript Protocols Demo    ║")
  (println "╚════════════════════════════════════════════════════════════════╝")

  ;; 1. Creating vectors
  (print-section "1. Creating Vectors")
  (let [v1 (pv/empty-vector)
        v2 (pv/vector 1 2 3)
        v3 (pv/from-array #js [10 20 30])]
    (println "  (pv/empty-vector)         =>" (str v1))
    (println "  (pv/vector 1 2 3)         =>" (str v2))
    (println "  (pv/from-array #js [...]) =>" (str v3)))

  ;; 2. Using count
  (print-section "2. Using count (ICounted)")
  (let [v (pv/vector 1 2 3 4 5)]
    (println "  (def v (pv/vector 1 2 3 4 5))")
    (println "  (count v) =>" (count v)))

  ;; 3. Using nth (IIndexed)
  (print-section "3. Using nth (IIndexed)")
  (let [v (pv/vector :a :b :c :d)]
    (println "  (def v (pv/vector :a :b :c :d))")
    (println "  (nth v 0)     =>" (nth v 0))
    (println "  (nth v 2)     =>" (nth v 2))
    (println "  (nth v 99 :x) =>" (nth v 99 :x)))

  ;; 4. Using first/last (via ISeqable/IStack)
  (print-section "4. Using first/last")
  (let [v (pv/vector "hello" "world" "!")]
    (println "  (def v (pv/vector \"hello\" \"world\" \"!\"))")
    (println "  (first v) =>" (first v))
    (println "  (last v)  =>" (last v)))

  ;; 5. Using conj (ICollection) - persistent operation
  (print-section "5. Using conj (ICollection) - Persistent!")
  (let [v1 (pv/vector 1 2 3)
        v2 (conj v1 4)
        v3 (conj v1 99)]
    (println "  (def v1 (pv/vector 1 2 3))")
    (println "  (def v2 (conj v1 4))")
    (println "  (def v3 (conj v1 99))")
    (println "")
    (println "  v1 =>" (str v1) "(unchanged!)")
    (println "  v2 =>" (str v2))
    (println "  v3 =>" (str v3)))

  ;; 6. Using pop (IStack) - persistent operation
  (print-section "6. Using pop (IStack) - Persistent!")
  (let [v1 (pv/vector 1 2 3 4)
        v2 (pop v1)]
    (println "  (def v1 (pv/vector 1 2 3 4))")
    (println "  (def v2 (pop v1))")
    (println "")
    (println "  v1 =>" (str v1) "(unchanged!)")
    (println "  v2 =>" (str v2)))

  ;; 7. Using assoc (IAssociative)
  (print-section "7. Using assoc (IAssociative)")
  (let [v1 (pv/vector :a :b :c)
        v2 (assoc v1 1 :REPLACED)]
    (println "  (def v1 (pv/vector :a :b :c))")
    (println "  (def v2 (assoc v1 1 :REPLACED))")
    (println "")
    (println "  v1 =>" (str v1) "(unchanged!)")
    (println "  v2 =>" (str v2)))

  ;; 8. Using as a function (IFn)
  (print-section "8. Using as a Function (IFn)")
  (let [v (pv/vector 100 200 300)]
    (println "  (def v (pv/vector 100 200 300))")
    (println "  (v 0) =>" (v 0))
    (println "  (v 2) =>" (v 2)))

  ;; 9. Using seq (ISeqable)
  (print-section "9. Using seq (ISeqable)")
  (let [v (pv/vector 1 2 3)]
    (println "  (def v (pv/vector 1 2 3))")
    (println "  (seq v) =>" (seq v))
    (println "  (type (seq v)) =>" (type (seq v))))

  ;; 10. Using reduce (IReduce)
  (print-section "10. Using reduce (IReduce)")
  (let [v (pv/vector 1 2 3 4 5)]
    (println "  (def v (pv/vector 1 2 3 4 5))")
    (println "  (reduce + v)    =>" (reduce + v))
    (println "  (reduce + 10 v) =>" (reduce + 10 v)))

  ;; 11. Using map/filter with native vectors
  (print-section "11. Using map/filter (via seq)")
  (let [v (pv/vector 1 2 3 4 5)]
    (println "  (def v (pv/vector 1 2 3 4 5))")
    (println "  (map inc v)          =>" (vec (map inc v)))
    (println "  (filter even? v)     =>" (vec (filter even? v)))
    (println "  (mapv #(* % %) v)    =>" (mapv #(* % %) v)))

  ;; 12. Equality
  (print-section "12. Equality (IEquiv)")
  (let [v1 (pv/vector 1 2 3)
        v2 (pv/vector 1 2 3)
        v3 (pv/vector 1 2 4)]
    (println "  (def v1 (pv/vector 1 2 3))")
    (println "  (def v2 (pv/vector 1 2 3))")
    (println "  (def v3 (pv/vector 1 2 4))")
    (println "")
    (println "  (= v1 v2) =>" (= v1 v2))
    (println "  (= v1 v3) =>" (= v1 v3))
    (println "  (= v1 [1 2 3]) =>" (= v1 [1 2 3])))

  ;; 13. Converting to/from ClojureScript vectors
  (print-section "13. Interop with ClojureScript Collections")
  (let [cljs-vec [1 2 3 4 5]
        native-v (pv/into-native cljs-vec)
        back-to-cljs (vec native-v)]
    (println "  (def cljs-vec [1 2 3 4 5])")
    (println "  (def native-v (pv/into-native cljs-vec))")
    (println "  (def back-to-cljs (vec native-v))")
    (println "")
    (println "  cljs-vec     =>" cljs-vec "(ClojureScript)")
    (println "  native-v     =>" (str native-v) "(Native)")
    (println "  back-to-cljs =>" back-to-cljs "(ClojureScript)"))

  ;; 14. Structural sharing demonstration
  (print-section "14. Structural Sharing")
  (let [base (pv/vector 1 2 3 4 5 6 7 8 9 10)
        derived1 (conj base 11)
        derived2 (conj base 12)
        derived3 (pop base)]
    (println "  (def base (pv/vector 1 2 3 4 5 6 7 8 9 10))")
    (println "  (def derived1 (conj base 11))")
    (println "  (def derived2 (conj base 12))")
    (println "  (def derived3 (pop base))")
    (println "")
    (println "  base     =>" (str base))
    (println "  derived1 =>" (str derived1))
    (println "  derived2 =>" (str derived2))
    (println "  derived3 =>" (str derived3))
    (println "")
    (println "  All four vectors share most of their structure in memory!")
    (println "  This is the power of persistent data structures."))

  ;; 15. Mixed types
  (print-section "15. Mixed Types Support")
  (let [v (pv/vector 42 "hello" true nil {:key "value"} [1 2 3])]
    (println "  (def v (pv/vector 42 \"hello\" true nil {:key \"value\"} [1 2 3]))")
    (println "  v =>" (str v))
    (println "")
    (println "  (nth v 0) =>" (nth v 0) "(number)")
    (println "  (nth v 1) =>" (nth v 1) "(string)")
    (println "  (nth v 2) =>" (nth v 2) "(boolean)")
    (println "  (nth v 3) =>" (nth v 3) "(nil)")
    (println "  (nth v 4) =>" (nth v 4) "(map)")
    (println "  (nth v 5) =>" (nth v 5) "(vector)"))

  (println "")
  (println "╔════════════════════════════════════════════════════════════════╗")
  (println "║                        Demo Complete!                          ║")
  (println "╚════════════════════════════════════════════════════════════════╝")
  (println ""))

;; Entry point
(defn init []
  (run-examples))

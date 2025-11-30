(ns hermes.persistent-map-example
  "Example program demonstrating native PersistentMap with ClojureScript protocols.

   This example shows how the native C++ persistent map can be used seamlessly
   with standard ClojureScript functions through protocol implementations.
   
   Also includes benchmarks comparing native map vs ClojureScript map performance."
  (:require [hermes.persistent-map :as pm]))

(defn print-section [title]
  (println)
  (println (str "=== " title " ===")))

;; Benchmarking utilities
(defn now-ms []
  "Returns current time in milliseconds."
  (js/performance.now))

(defn benchmark
  "Run f n times and return elapsed time in ms."
  [n f]
  (let [start (now-ms)]
    (dotimes [_ n]
      (f))
    (- (now-ms) start)))

(defn format-time [ms]
  "Format milliseconds nicely."
  (if (< ms 1)
    (str (.toFixed (* ms 1000) 2) "µs")
    (str (.toFixed ms 2) "ms")))

(defn run-examples []
  (println "")
  (println "╔════════════════════════════════════════════════════════════════╗")
  (println "║   Native PersistentMap with ClojureScript Protocols Demo      ║")
  (println "╚════════════════════════════════════════════════════════════════╝")

  ;; 1. Creating maps
  (print-section "1. Creating Maps")
  (let [m1 (pm/empty-map)
        m2 (pm/map :x 1 :y 2 :z 3)
        m3 (pm/from-object #js {:a 10 :b 20 :c 30})]
    (println "  (pm/empty-map)              =>" (str m1))
    (println "  (pm/map :x 1 :y 2 :z 3)    =>" (str m2))
    (println "  (pm/from-object #js {...})  =>" (str m3)))

  ;; 2. Using count (ICounted)
  (print-section "2. Using count (ICounted)")
  (let [m (pm/map :a 1 :b 2 :c 3 :d 4 :e 5)]
    (println "  (def m (pm/map :a 1 :b 2 :c 3 :d 4 :e 5))")
    (println "  (count m) =>" (count m)))

  ;; 3. Using get (ILookup)
  (print-section "3. Using get (ILookup)")
  (let [m (pm/map :name "Alice" :age 30 :city "NYC")]
    (println "  (def m (pm/map :name \"Alice\" :age 30 :city \"NYC\"))")
    (println "  (get m :name)    =>" (get m :name))
    (println "  (get m :age)     =>" (get m :age))
    (println "  (get m :unknown :DEFAULT) =>" (get m :unknown :DEFAULT)))

  ;; 4. Using map as a function (IFn via ILookup)
  (print-section "4. Using Map as a Function")
  (let [m (pm/map :x 100 :y 200 :z 300)]
    (println "  (def m (pm/map :x 100 :y 200 :z 300))")
    (println "  (m :x)           =>" (m :x))
    (println "  (m :z)           =>" (m :z))
    (println "  (m :missing :NA) =>" (m :missing :NA)))

  ;; 5. Using assoc (IAssociative) - persistent operation
  (print-section "5. Using assoc (IAssociative) - Persistent!")
  (let [m1 (pm/map :a 1 :b 2)
        m2 (assoc m1 :c 3)
        m3 (assoc m1 :a 99)]
    (println "  (def m1 (pm/map :a 1 :b 2))")
    (println "  (def m2 (assoc m1 :c 3))")
    (println "  (def m3 (assoc m1 :a 99))")
    (println "")
    (println "  m1 =>" (str m1) "(unchanged!)")
    (println "  m2 =>" (str m2))
    (println "  m3 =>" (str m3)))

  ;; 6. Using dissoc (IAssociative) - persistent operation
  (print-section "6. Using dissoc (IAssociative) - Persistent!")
  (let [m1 (pm/map :a 1 :b 2 :c 3)
        m2 (dissoc m1 :b)]
    (println "  (def m1 (pm/map :a 1 :b 2 :c 3))")
    (println "  (def m2 (dissoc m1 :b))")
    (println "")
    (println "  m1 =>" (str m1) "(unchanged!)")
    (println "  m2 =>" (str m2)))

  ;; 7. Using conj (ICollection) - add [key value] pairs
  (print-section "7. Using conj (ICollection)")
  (let [m1 (pm/map :a 1 :b 2)
        m2 (conj m1 [:c 3])
        m3 (conj m1 [:a 99])]
    (println "  (def m1 (pm/map :a 1 :b 2))")
    (println "  (def m2 (conj m1 [:c 3]))")
    (println "  (def m3 (conj m1 [:a 99]))")
    (println "")
    (println "  m1 =>" (str m1) "(unchanged!)")
    (println "  m2 =>" (str m2))
    (println "  m3 =>" (str m3)))

  ;; 8. Using keys/vals
  (print-section "8. Using keys/vals (IKeys/IVals)")
  (let [m (pm/map :a 10 :b 20 :c 30)]
    (println "  (def m (pm/map :a 10 :b 20 :c 30))")
    (println "  (.-m m) type:" (type (.-m m)))
    (let [entries (.entries js/PersistentMap (.-m m))]
      (println "  entries from .entries():" entries)
      (println "  entries length:" (.-length entries))
      (when (> (.-length entries) 0)
        (println "  first entry:" (aget entries 0))
        (println "  first entry[0]:" (aget (aget entries 0) 0))
        (println "  first entry[1]:" (aget (aget entries 0) 1))))
    (println "  (seq m) =>" (seq m))
    (println "  (keys m) =>" (vec (keys m)))
    (println "  (vals m) =>" (vec (vals m))))

  ;; 9. Using seq (ISeqable)
  (print-section "9. Using seq (ISeqable)")
  (let [m (pm/map :x 1 :y 2 :z 3)]
    (println "  (def m (pm/map :x 1 :y 2 :z 3))")
    (println "  (seq m)        =>" (vec (seq m)))
    (println "  (type (seq m)) =>" (type (seq m))))

  ;; 10. Using reduce (IReduce)
  (print-section "10. Using reduce (IReduce)")
  (let [m (pm/map :a 10 :b 20 :c 30 :d 40)]
    (println "  (def m (pm/map :a 10 :b 20 :c 30 :d 40))")
    (println "  (reduce + (vals m))      =>" (reduce + (vals m)))
    (println "  (reduce + 100 (vals m))  =>" (reduce + 100 (vals m))))

  ;; 11. Map with arbitrary keys (not just keywords)
  (print-section "11. Maps with Arbitrary Key Types")
  (let [m1 (pm/map :keyword-key 1
                   "string-key" 2
                   42 "numeric-key"
                   :nested {:obj true}
                   {:x 1} "hello")]
    (println "  (def m (pm/map :keyword-key 1")
    (println "                  \"string-key\" 2")
    (println "                  42 \"numeric-key\"")
    (println "                  :nested {:obj true} \"object-key\"))")
    (println "")
    (println "  (get m :keyword-key)  =>" (get m1 :keyword-key))
    (println "  (get m \"string-key\") =>" (get m1 "string-key"))
    (println "  (get m 42)             =>" (get m1 42))
    (println "  (get m :nested)        =>" (get m1 :nested))
    (println "  (get m {:x 1})         =>" (get m1 {:x 1})))

  ;; 12. Equality
  (print-section "12. Equality (IEquiv)")
  (let [m1 (pm/map :a 1 :b 2)
        m2 (pm/map :b 2 :a 1)
        m3 (pm/map :a 1 :b 99)]
    (println "  (def m1 (pm/map :a 1 :b 2))")
    (println "  (def m2 (pm/map :b 2 :a 1))")
    (println "  (def m3 (pm/map :a 1 :b 99))")
    (println "")
    (println "  (= m1 m2) =>" (= m1 m2))
    (println "  (= m1 m3) =>" (= m1 m3))
    (println "  (= m1 {:a 1 :b 2}) =>" (= m1 {:a 1 :b 2})))

  ;; 13. Converting to/from ClojureScript maps
  (print-section "13. Interop with ClojureScript Collections")
  (let [cljs-map {:x 100 :y 200 :z 300}
        native-m (pm/into-native cljs-map)
        back-to-cljs (into {} native-m)]
    (println "  (def cljs-map {:x 100 :y 200 :z 300})")
    (println "  (def native-m (pm/into-native cljs-map))")
    (println "  (into {} native-m)")
    (println "")
    (println "  CLJS map:   " (str cljs-map))
    (println "  Native map: " (str native-m))
    (println "  Back to CLJS:" (str back-to-cljs)))

  ;; 14. Merge maps
  (print-section "14. Merging Maps")
  (let [m1 (pm/map :a 1 :b 2)
        m2 (pm/map :b 99 :c 3)
        merged (merge m1 m2)]
    (println "  (def m1 (pm/map :a 1 :b 2))")
    (println "  (def m2 (pm/map :b 99 :c 3))")
    (println "  (merge m1 m2)")
    (println "")
    (println "  m1:     " (str m1))
    (println "  m2:     " (str m2))
    (println "  merged: " (str merged)))

  ;; BENCHMARKS
  (println "\n")
  (println "╔════════════════════════════════════════════════════════════════╗")
  (println "║                         BENCHMARKS                            ║")
  (println "╚════════════════════════════════════════════════════════════════╝")

  (let [map-size 100
        iterations 1000]

    ;; Benchmark 1: Map creation
    (print-section "Benchmark 1: Map Creation (from object)")
    (let [native-time (benchmark iterations
                        #(pm/from-object #js {:a 1 :b 2 :c 3}))
          cljs-time (benchmark iterations
                      #(hash-map :a 1 :b 2 :c 3))]
      (println (str "  Native map: " (format-time native-time)))
      (println (str "  CLJS map:   " (format-time cljs-time)))
      (println (str "  Ratio:      " (.toFixed (/ native-time cljs-time) 2) "x")))

    ;; Benchmark 2: assoc sequential
    (print-section "Benchmark 2: Sequential assoc (building map)")
    (let [native-time (benchmark 100
                        #(loop [m (pm/empty-map) i 0]
                           (if (< i map-size)
                             (recur (assoc m (keyword (str "key" i)) i) (inc i))
                             m)))
          cljs-time (benchmark 100
                      #(loop [m {} i 0]
                         (if (< i map-size)
                           (recur (assoc m (keyword (str "key" i)) i) (inc i))
                           m)))]
      (println (str "  Native map: " (format-time native-time)))
      (println (str "  CLJS map:   " (format-time cljs-time)))
      (println (str "  Ratio:      " (.toFixed (/ native-time cljs-time) 2) "x")))

    ;; Benchmark 3: get operations
    (print-section "Benchmark 3: Random get operations")
    (let [native-m (loop [m (pm/empty-map) i 0]
                     (if (< i map-size)
                       (recur (assoc m (keyword (str "key" i)) i) (inc i))
                       m))
          cljs-m (loop [m {} i 0]
                   (if (< i map-size)
                     (recur (assoc m (keyword (str "key" i)) i) (inc i))
                     m))
          native-time (benchmark iterations
                        #(get native-m (keyword (str "key" (rand-int map-size)))))
          cljs-time (benchmark iterations
                      #(get cljs-m (keyword (str "key" (rand-int map-size)))))]
      (println (str "  Native map: " (format-time native-time)))
      (println (str "  CLJS map:   " (format-time cljs-time)))
      (println (str "  Ratio:      " (.toFixed (/ native-time cljs-time) 2) "x")))

    ;; Benchmark 4: keys extraction
    (print-section "Benchmark 4: Extract keys")
    (let [native-m (loop [m (pm/empty-map) i 0]
                     (if (< i map-size)
                       (recur (assoc m (keyword (str "key" i)) i) (inc i))
                       m))
          cljs-m (loop [m {} i 0]
                   (if (< i map-size)
                     (recur (assoc m (keyword (str "key" i)) i) (inc i))
                     m))
          native-time (benchmark iterations #(keys native-m))
          cljs-time (benchmark iterations #(keys cljs-m))]
      (println (str "  Native map: " (format-time native-time)))
      (println (str "  CLJS map:   " (format-time cljs-time)))
      (println (str "  Ratio:      " (.toFixed (/ native-time cljs-time) 2) "x")))

    ;; Benchmark 5: seq iteration
    (print-section "Benchmark 5: seq iteration")
    (let [native-m (loop [m (pm/empty-map) i 0]
                     (if (< i map-size)
                       (recur (assoc m (keyword (str "key" i)) i) (inc i))
                       m))
          cljs-m (loop [m {} i 0]
                   (if (< i map-size)
                     (recur (assoc m (keyword (str "key" i)) i) (inc i))
                     m))
          native-time (benchmark iterations
                        #(reduce (fn [acc [k v]] (+ acc v)) 0 native-m))
          cljs-time (benchmark iterations
                      #(reduce (fn [acc [k v]] (+ acc v)) 0 cljs-m))]
      (println (str "  Native map: " (format-time native-time)))
      (println (str "  CLJS map:   " (format-time cljs-time)))
      (println (str "  Ratio:      " (.toFixed (/ native-time cljs-time) 2) "x"))))

  (println "\n✓ All examples completed!"))

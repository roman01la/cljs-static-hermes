(ns hermes.persistent-vector-example
  "Example program demonstrating native PersistentVector with ClojureScript protocols.

   This example shows how the native C++ persistent vector can be used seamlessly
   with standard ClojureScript functions through protocol implementations.
   
   Also includes benchmarks comparing native vector vs ClojureScript vector performance."
  (:require [hermes.persistent-vector :as pv]))

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

(defn run-benchmarks []
  (println "")
  (println "╔════════════════════════════════════════════════════════════════╗")
  (println "║     Performance Benchmarks: Native Vector vs CLJS Vector      ║")
  (println "╚════════════════════════════════════════════════════════════════╝")
  (println "")
  (println "Running benchmarks... (this may take a moment)")
  (println "")
  
  (let [iterations 10000
        small-size 10
        medium-size 100
        large-size 1000]
    
    ;; ========================================
    ;; Benchmark 1: Vector Creation
    ;; ========================================
    (print-section "Benchmark 1: Vector Creation")
    (println (str "  Creating vectors with " small-size " elements, " iterations " iterations"))
    (println "")
    
    (let [js-arr (apply array (range small-size))
          
          cljs-time (benchmark iterations #(vec (range small-size)))
          native-time (benchmark iterations #(pv/from-array js-arr))]
      (println (str "  CLJS vector:   " (format-time cljs-time)))
      (println (str "  Native vector: " (format-time native-time)))
      (println (str "  Ratio: " (.toFixed (/ cljs-time native-time) 2) "x")))
    
    ;; ========================================
    ;; Benchmark 2: conj (append) operations
    ;; ========================================
    (print-section "Benchmark 2: conj (append) - Sequential")
    (println (str "  Appending " medium-size " elements one at a time"))
    (println "")
    
    (let [cljs-time (benchmark 100
                      #(loop [v [] i 0]
                         (if (< i medium-size)
                           (recur (conj v i) (inc i))
                           v)))
          native-time (benchmark 100
                        #(loop [v (pv/empty-vector) i 0]
                           (if (< i medium-size)
                             (recur (conj v i) (inc i))
                             v)))]
      (println (str "  CLJS vector:   " (format-time cljs-time)))
      (println (str "  Native vector: " (format-time native-time)))
      (println (str "  Ratio: " (.toFixed (/ cljs-time native-time) 2) "x")))
    
    ;; ========================================
    ;; Benchmark 3: nth (random access)
    ;; ========================================
    (print-section "Benchmark 3: nth (random access)")
    (println (str "  Accessing random elements from vector of size " large-size))
    (println "")
    
    (let [cljs-v (vec (range large-size))
          native-v (pv/from-array (apply array (range large-size)))
          ;; Pre-generate indices as a vector for consistent memory usage
          indices (vec (repeatedly iterations #(rand-int large-size)))
          
          cljs-time (benchmark 1 #(doseq [i indices] (nth cljs-v i)))
          native-time (benchmark 1 #(doseq [i indices] (nth native-v i)))]
      (println (str "  CLJS vector:   " (format-time cljs-time) " (" iterations " accesses)"))
      (println (str "  Native vector: " (format-time native-time) " (" iterations " accesses)"))
      (println (str "  Ratio: " (.toFixed (/ cljs-time native-time) 2) "x")))
    
    ;; ========================================
    ;; Benchmark 4: count
    ;; ========================================
    (print-section "Benchmark 4: count")
    (println (str "  Getting count of vector with " large-size " elements"))
    (println "")
    
    (let [cljs-v (vec (range large-size))
          native-v (pv/from-array (apply array (range large-size)))
          
          cljs-time (benchmark iterations #(count cljs-v))
          native-time (benchmark iterations #(count native-v))]
      (println (str "  CLJS vector:   " (format-time cljs-time)))
      (println (str "  Native vector: " (format-time native-time)))
      (println (str "  Ratio: " (.toFixed (/ cljs-time native-time) 2) "x")))
    
    ;; ========================================
    ;; Benchmark 5: pop
    ;; ========================================
    (print-section "Benchmark 5: pop (remove last)")
    (println (str "  Popping " medium-size " elements one at a time"))
    (println "")
    
    (let [cljs-base (vec (range medium-size))
          native-base (pv/from-array (apply array (range medium-size)))
          
          cljs-time (benchmark 100
                      #(loop [v cljs-base]
                         (if (pos? (count v))
                           (recur (pop v))
                           v)))
          native-time (benchmark 100
                        #(loop [v native-base]
                           (if (pos? (count v))
                             (recur (pop v))
                             v)))]
      (println (str "  CLJS vector:   " (format-time cljs-time)))
      (println (str "  Native vector: " (format-time native-time)))
      (println (str "  Ratio: " (.toFixed (/ cljs-time native-time) 2) "x")))
    
    ;; ========================================
    ;; Benchmark 6: assoc (update)
    ;; ========================================
    (print-section "Benchmark 6: assoc (update at index)")
    (println (str "  Updating random indices in vector of size " medium-size))
    (println "")
    
    (let [cljs-v (vec (range medium-size))
          native-v (pv/from-array (apply array (range medium-size)))
          ;; Pre-generate random indices to avoid timing overhead
          indices (vec (repeatedly iterations #(rand-int medium-size)))
          
          cljs-time (benchmark 1 #(doseq [i indices] (assoc cljs-v i :updated)))
          native-time (benchmark 1 #(doseq [i indices] (assoc native-v i :updated)))]
      (println (str "  CLJS vector:   " (format-time cljs-time)))
      (println (str "  Native vector: " (format-time native-time)))
      (println (str "  Ratio: " (.toFixed (/ cljs-time native-time) 2) "x")))
    
    ;; ========================================
    ;; Benchmark 7: reduce (iteration)
    ;; ========================================
    (print-section "Benchmark 7: reduce (sum all elements)")
    (println (str "  Summing vector with " large-size " elements"))
    (println "")
    
    (let [cljs-v (vec (range large-size))
          native-v (pv/from-array (apply array (range large-size)))
          
          cljs-time (benchmark 1000 #(reduce + 0 cljs-v))
          native-time (benchmark 1000 #(reduce + 0 native-v))]
      (println (str "  CLJS vector:   " (format-time cljs-time)))
      (println (str "  Native vector: " (format-time native-time)))
      (println (str "  Ratio: " (.toFixed (/ cljs-time native-time) 2) "x")))
    
    ;; ========================================
    ;; Benchmark 8: first/last
    ;; ========================================
    (print-section "Benchmark 8: first and last (using peek for last)")
    (println (str "  Getting first/peek from vector of size " large-size))
    (println "  Note: Using 'peek' instead of 'last' for efficient last-element access")
    (println "")
    
    (let [cljs-v (vec (range large-size))
          native-v (pv/from-array (apply array (range large-size)))
          
          cljs-first-time (benchmark iterations #(first cljs-v))
          native-first-time (benchmark iterations #(first native-v))
          ;; Use peek for efficient last-element access (O(1) for both)
          cljs-peek-time (benchmark iterations #(peek cljs-v))
          native-peek-time (benchmark iterations #(peek native-v))]
      (println "  first:")
      (println (str "    CLJS vector:   " (format-time cljs-first-time)))
      (println (str "    Native vector: " (format-time native-first-time)))
      (println (str "    Ratio: " (.toFixed (/ cljs-first-time native-first-time) 2) "x"))
      (println "")
      (println "  peek (last element):")
      (println (str "    CLJS vector:   " (format-time cljs-peek-time)))
      (println (str "    Native vector: " (format-time native-peek-time)))
      (println (str "    Ratio: " (.toFixed (/ cljs-peek-time native-peek-time) 2) "x")))
    
    ;; ========================================
    ;; Benchmark 9: Large vector operations
    ;; ========================================
    (print-section "Benchmark 9: Large Vector - Mixed Operations")
    (println (str "  Building then querying vector with " large-size " elements"))
    (println "")
    
    (let [build-and-query-cljs 
          (fn []
            (let [v (loop [v [] i 0]
                      (if (< i large-size)
                        (recur (conj v i) (inc i))
                        v))]
              ;; Query phase
              (dotimes [_ 100]
                (nth v (rand-int large-size))
                (count v))))
          
          build-and-query-native
          (fn []
            (let [v (loop [v (pv/empty-vector) i 0]
                      (if (< i large-size)
                        (recur (conj v i) (inc i))
                        v))]
              ;; Query phase  
              (dotimes [_ 100]
                (nth v (rand-int large-size))
                (count v))))
          
          cljs-time (benchmark 10 build-and-query-cljs)
          native-time (benchmark 10 build-and-query-native)]
      (println (str "  CLJS vector:   " (format-time cljs-time)))
      (println (str "  Native vector: " (format-time native-time)))
      (println (str "  Ratio: " (.toFixed (/ cljs-time native-time) 2) "x"))))
  
  (println "")
  (println "╔════════════════════════════════════════════════════════════════╗")
  (println "║                    Benchmarks Complete!                        ║")
  (println "║                                                                ║")
  (println "║  Note: Ratio > 1 means Native is faster                        ║")
  (println "║        Ratio < 1 means CLJS is faster                          ║")
  (println "║                                                                ║")
  (println "║  Results may vary based on vector size, operation mix,         ║")
  (println "║  and runtime environment.                                      ║")
  (println "╚════════════════════════════════════════════════════════════════╝")
  (println ""))

;; Entry point
(defn init []
  (run-examples)
  (run-benchmarks))

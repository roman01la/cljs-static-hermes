(ns hermes.persistent-map
  "ClojureScript wrapper for the native PersistentMap implemented in C++.

   This namespace provides a ClojureScript-compatible interface to the native
   persistent map exposed via Hermes JSI. It implements the core collection
   protocols so that the native map can be used with standard ClojureScript
   functions like `count`, `get`, `assoc`, `dissoc`, etc.

   Usage:
     (require '[hermes.persistent-map :as pm])

     ;; Create maps
     (def m1 (pm/empty-map))
     (def m2 (pm/map :x 1 :y 2))
     (def m3 (pm/from-object #js {:x 1 :y 2}))

     ;; Use with standard ClojureScript functions
     (count m2)        ;; => 2
     (get m2 :x)       ;; => 1
     (assoc m2 :z 3)   ;; => native map {:x 1 :y 2 :z 3}
     (dissoc m2 :x)    ;; => native map {:y 2}
     (seq m2)          ;; => ([:x 1] [:y 2])

   The native map provides O(log n) structural sharing for efficient
   persistent operations, backed by the Immer C++ library."
  (:refer-clojure :exclude [map])
  (:require [clojure.string :as str]))

;; Access the native PersistentMap factory from the global scope
(def ^:private native-factory js/PersistentMap)

;; Helper to check if something is a native persistent map
(defn native-map?
  "Returns true if x is a native PersistentMap."
  [x]
  (instance? js/PersistentMap x))

;; Wrapper type that implements ClojureScript protocols
(deftype NativePersistentMap [m cnt meta]
  Object
  (toString [_]
    (let [entries (.entries native-factory m)]
      (str "{"
           (str/join ", "
                     (cljs.core/map #(str (first %) " " (second %)) entries))
           "}")))
  (equiv [this other]
    (-equiv this other))

  ICounted
  (-count [_]
    cnt)

  ILookup
  (-lookup [this k]
    (-lookup this k nil))
  (-lookup [this k not-found]
    (let [result (.get native-factory m k)]
      (if (undefined? result) not-found result)))

  ICollection
  (-conj [this entry]
    (if (vector? entry)
      (if (= (count entry) 2)
        (let [k (nth entry 0)
              v (nth entry 1)]
          (NativePersistentMap. (.assoc native-factory m k v) (inc cnt) meta))
        (throw (js/Error. "Entry must be a [key value] pair")))
      (loop [ret this es (seq entry)]
        (if (nil? es)
          ret
          (let [e (first es)]
            (if (vector? e)
              (recur (-conj ret e) (next es))
              (throw (js/Error. "conj on a map takes map entries or seqables of map entries"))))))))

  IEmptyableCollection
  (-empty [_]
    (NativePersistentMap. (.empty native-factory) 0 nil))

  IEquiv
  (-equiv [this other]
    (cond
      (instance? NativePersistentMap other)
      (.equiv native-factory m (.-m other))

      (map? other)
      (and (= (count this) (count other))
           (every? (fn [[k v]] (= v (get other k)))
                   (seq this)))

      :else false))

  IHash
  (-hash [this]
    (hash (into {} (seq this))))

  ISeqable
  (-seq [this]
    (when (pos? cnt)
      (let [entries (.entries native-factory m)]
        (cljs.core/map (fn [entry]
               (MapEntry. (aget entry 0) (aget entry 1) nil))
             entries))))

  IAssociative
  (-assoc [this k v]
    (NativePersistentMap. (.assoc native-factory m k v) cnt meta))
  (-contains-key? [this k]
    (.has native-factory m k))

  IFind
  (-find [this k]
    (when (.has native-factory m k)
      (cljs.core/MapEntry. k (get this k) nil)))

  IMap
  (-dissoc [this k]
    (NativePersistentMap. (.dissoc native-factory m k) (dec cnt) meta))

  IKVReduce
  (-kv-reduce [this f init]
    (loop [result init
           entries (.entries native-factory m)]
      (if (empty? entries)
        result
        (let [[k v] (first entries)]
          (if (reduced? result)
            @result
            (recur (f result k v) (rest entries)))))))

  IFn
  (-invoke [this k]
    (-lookup this k))
  (-invoke [this k not-found]
    (-lookup this k not-found))

  IMeta
  (-meta [_]
    meta)

  IWithMeta
  (-with-meta [this new-meta]
    (if (identical? new-meta meta)
      this
      (NativePersistentMap. m cnt new-meta)))

  ICloneable
  (-clone [_] (NativePersistentMap. m cnt meta))

  IPrintWithWriter
  (-pr-writer [this writer opts]
    (-write writer "#hermes/pmap ")
    (-write writer (str this))))

;; Factory functions

(defn empty-map
  "Returns an empty native persistent map."
  []
  (NativePersistentMap. (.empty native-factory) 0 nil))

(defn from-object
  "Creates a native persistent map from a JavaScript object."
  [obj]
  (let [entries (.entries native-factory (.from native-factory obj))
        cnt (count entries)]
    (NativePersistentMap. (.from native-factory obj) cnt nil)))

(defn map
  "Creates a native persistent map containing the given key-value pairs.
   Supports arbitrary keys (keywords, strings, numbers, objects), not just string keys."
  [& kvs]
  (if (seq kvs)
    (loop [result (.empty native-factory)
           i 0
           cnt 0]
      (if (< i (count kvs))
        (recur (.assoc native-factory result (nth kvs i) (nth kvs (inc i)))
               (+ i 2)
               (inc cnt))
        (NativePersistentMap. result cnt nil)))
    (empty-map)))

(defn ->native
  "If x is a NativePersistentMap, returns the underlying native map.
   Otherwise returns x unchanged."
  [x]
  (if (instance? NativePersistentMap x)
    (.-m x)
    x))

(defn into-native
  "Converts a ClojureScript map into a native persistent map."
  [coll]
  (let [obj (js-obj)]
    (doseq [[k v] coll]
      (aset obj (name k) v))
    (from-object obj)))

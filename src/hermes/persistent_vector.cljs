(ns hermes.persistent-vector
  "ClojureScript wrapper for the native PersistentVector implemented in C++.

   This namespace provides a ClojureScript-compatible interface to the native
   persistent vector exposed via Hermes JSI. It implements the core collection
   protocols so that the native vector can be used with standard ClojureScript
   functions like `count`, `nth`, `first`, `last`, `conj`, `pop`, etc.

   Usage:
     (require '[hermes.persistent-vector :as pv])

     ;; Create vectors
     (def v1 (pv/empty-vector))
     (def v2 (pv/vector 1 2 3))
     (def v3 (pv/from-array #js [1 2 3]))

     ;; Use with standard ClojureScript functions
     (count v2)        ;; => 3
     (nth v2 0)        ;; => 1
     (first v2)        ;; => 1
     (last v2)         ;; => 3
     (conj v2 4)       ;; => native vector [1 2 3 4]
     (pop v2)          ;; => native vector [1 2]
     (seq v2)          ;; => (1 2 3)

   The native vector provides O(log n) structural sharing for efficient
   persistent operations, backed by the Immer C++ library."
  (:require [clojure.string :as str]))

;; Access the native PersistentVector factory from the global scope
(def ^:private native-factory js/PersistentVector)

;; Helper to check if something is a native persistent vector
(defn native-vector?
  "Returns true if x is a native PersistentVector."
  [x]
  (and (some? x)
       (fn? (.-count x))
       (fn? (.-nth x))
       (fn? (.-conj x))
       (fn? (.-pop x))))

;; Wrapper type that implements ClojureScript protocols
(deftype NativePersistentVector [native]
  Object
  (toString [_]
    (str "[" (str/join " " (.toArray native)) "]"))

  ICounted
  (-count [_]
    (.count native))

  IIndexed
  (-nth [_ n]
    (.nth native n))
  (-nth [_ n not-found]
    (if (and (>= n 0) (< n (.count native)))
      (.nth native n)
      not-found))

  ILookup
  (-lookup [this k]
    (-lookup this k nil))
  (-lookup [_ k not-found]
    (if (and (number? k) (>= k 0) (< k (.count native)))
      (.nth native k)
      not-found))

  ICollection
  (-conj [_ v]
    (NativePersistentVector. (.conj native v)))

  IEmptyableCollection
  (-empty [_]
    (NativePersistentVector. (.empty native-factory)))

  IStack
  (-peek [_]
    (when (pos? (.count native))
      (.last native)))
  (-pop [_]
    (NativePersistentVector. (.pop native)))

  ISeqable
  (-seq [_]
    (when (pos? (.count native))
      (map #(.nth native %) (range (.count native)))))

  ISequential

  IEquiv
  (-equiv [this other]
    (cond
      (instance? NativePersistentVector other)
      (= (.toArray native) (.toArray (.-native other)))

      (sequential? other)
      (= (seq this) (seq other))

      :else false))

  IHash
  (-hash [this]
    (hash (into [] this)))

  IReduce
  (-reduce [this f]
    (if (zero? (.count native))
      (f)
      (loop [i 1
             acc (.nth native 0)]
        (if (< i (.count native))
          (let [result (f acc (.nth native i))]
            (if (reduced? result)
              @result
              (recur (inc i) result)))
          acc))))
  (-reduce [_ f init]
    (loop [i 0
           acc init]
      (if (< i (.count native))
        (let [result (f acc (.nth native i))]
          (if (reduced? result)
            @result
            (recur (inc i) result)))
        acc)))

  IAssociative
  (-assoc [_ k v]
    (if (and (number? k) (>= k 0) (< k (.count native)))
      (NativePersistentVector. (.assoc native k v))
      (throw (js/Error. (str "Index " k " out of bounds for vector of size " (.count native))))))
  (-contains-key? [_ k]
    (and (number? k) (>= k 0) (< k (.count native))))

  IFind
  (-find [_ k]
    (when (and (number? k) (>= k 0) (< k (.count native)))
      (cljs.core/MapEntry. k (.nth native k) nil)))

  IVector
  (-assoc-n [_ n val]
    (if (and (>= n 0) (< n (.count native)))
      (NativePersistentVector. (.assoc native n val))
      (throw (js/Error. (str "Index " n " out of bounds for vector of size " (.count native))))))

  IFn
  (-invoke [this k]
    (-nth this k))
  (-invoke [this k not-found]
    (-nth this k not-found))

  IPrintWithWriter
  (-pr-writer [this writer opts]
    (-write writer "#hermes/pvec ")
    (-write writer (str this))))

;; Factory functions

(defn empty-vector
  "Returns an empty native persistent vector."
  []
  (NativePersistentVector. (.empty native-factory)))

(defn from-array
  "Creates a native persistent vector from a JavaScript array."
  [arr]
  (NativePersistentVector. (.from native-factory arr)))

(defn vector
  "Creates a native persistent vector containing the given values."
  [& args]
  (if (seq args)
    (from-array (to-array args))
    (empty-vector)))

(defn ->native
  "If x is a NativePersistentVector, returns the underlying native vector.
   Otherwise returns x unchanged."
  [x]
  (if (instance? NativePersistentVector x)
    (.-native x)
    x))

(defn into-native
  "Converts a ClojureScript collection into a native persistent vector."
  [coll]
  (from-array (to-array coll)))

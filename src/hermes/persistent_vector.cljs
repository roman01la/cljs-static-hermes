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
  (:refer-clojure :exclude [vector])
  (:require [clojure.string :as str]))

;; Access the native PersistentVector factory from the global scope
(def ^:private native-factory js/PersistentVector)

;; Helper to check if something is a native persistent vector
(defn native-vector?
  "Returns true if x is a native PersistentVector."
  [x]
  (instance? js/PersistentVector x))

;; Wrapper type that implements ClojureScript protocols
(deftype NativePersistentVector [v cnt meta]
  Object
  (toString [_]
    (str "[" (str/join " " (.toArray native-factory v)) "]"))
  (equiv [this other]
    (-equiv this other))

  ICounted
  (-count [_]
    cnt)

  IIndexed
  (-nth [this n]
    (.nth native-factory v n))
  (-nth [this n not-found]
    (if (and (>= n 0) (< n cnt))
      (-nth this n)
      not-found))

  ILookup
  (-lookup [this k]
    (-lookup this k nil))
  (-lookup [this k not-found]
    (if (and (number? k) (>= k 0) (< k cnt))
      (-nth this k)
      not-found))

  ICollection
  (-conj [_ elem]
    (NativePersistentVector. (.conj native-factory v elem) nil meta))

  IEmptyableCollection
  (-empty [_]
    (NativePersistentVector. (.empty native-factory) 0 nil))

  IStack
  (-peek [this]
    (when (pos? cnt)
      (.last native-factory v)))
  (-pop [_]
    (NativePersistentVector. (.pop native-factory v) (dec cnt) meta))

  ISeqable
  (-seq [this]
    (when (pos? cnt)
      ;; Use native toArray for efficient seq conversion
      (seq (.toArray native-factory v))))

  ISequential

  IEquiv
  (-equiv [this other]
    (cond
      (instance? NativePersistentVector other)
      (.equiv native-factory v (.-v other))

      (sequential? other)
      (= (seq this) (seq other))

      :else false))

  IHash
  (-hash [this]
    (hash (into [] this)))

  IReduce
  (-reduce [this f]
    (.reduce native-factory v #(f %1 %2) nil))
  (-reduce [this f init]
    (.reduce native-factory v #(f %1 %2) init))

  IAssociative
  (-assoc [this k val]
    (NativePersistentVector. (.assoc native-factory v k val) cnt meta))
  (-contains-key? [this k]
    (and (integer? k) (>= k 0) (< k cnt)))

  IFind
  (-find [this k]
    (when (and (>= k 0) (< k cnt))
      (cljs.core/MapEntry. k (-nth this k) nil)))

  INext
  (-next [this]
    (when (> cnt 1)
      ;; Return rest of the seq from toArray
      (next (seq (.toArray native-factory (.-v this))))))

  APersistentVector
  IVector
  (-assoc-n [this n val]
    (NativePersistentVector. (.assoc native-factory v n val) cnt meta))

  IFn
  (-invoke [coll k]
    (if (number? k)
      (-nth coll k)
      (throw (js/Error. "Key must be integer"))))

  IMeta
  (-meta [_]
    meta)

  IWithMeta
  (-with-meta [this new-meta]
    (if (identical? new-meta meta)
      this
      (NativePersistentVector. v cnt new-meta)))

  ICloneable
  (-clone [_] (NativePersistentVector. v cnt meta))

  IDrop
  (-drop [this n]
    (if (< n cnt)
      (let [remaining (- cnt n)]
        ;; Create a lazy sequence starting from index n
        (map #(-nth this (+ n %)) (range remaining)))
      nil))

  IPrintWithWriter
  (-pr-writer [this writer opts]
    (-write writer "#hermes/pvec ")
    (-write writer (str this))))

;; Factory functions

(defn empty-vector
  "Returns an empty native persistent vector."
  []
  (NativePersistentVector. (.empty native-factory) 0 nil))

(defn from-array
  "Creates a native persistent vector from a JavaScript array."
  [arr]
  (NativePersistentVector. (.from native-factory arr) (.-length arr) nil))

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

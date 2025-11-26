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
;; We cache method references to avoid repeated JSI property lookups
(deftype NativePersistentVector [native
                                  ^:mutable -cached-count
                                  ^:mutable -cached-nth
                                  ^:mutable -cached-conj
                                  ^:mutable -cached-pop
                                  ^:mutable -cached-assoc
                                  ^:mutable -cached-first
                                  ^:mutable -cached-last
                                  ^:mutable -cached-toArray
                                  ^:mutable -cached-empty]
  Object
  (toString [_]
    (let [to-arr (or -cached-toArray
                     (set! -cached-toArray (.-toArray native)))]
      (str "[" (str/join " " (.call to-arr native)) "]")))

  ICounted
  (-count [_]
    (let [cnt (or -cached-count
                  (set! -cached-count (.-count native)))]
      (.call cnt native)))

  IIndexed
  (-nth [this n]
    (let [nth-fn (or -cached-nth
                     (set! -cached-nth (.-nth native)))]
      (.call nth-fn native n)))
  (-nth [this n not-found]
    (let [cnt (or -cached-count
                  (set! -cached-count (.-count native)))]
      (if (and (>= n 0) (< n (.call cnt native)))
        (let [nth-fn (or -cached-nth
                         (set! -cached-nth (.-nth native)))]
          (.call nth-fn native n))
        not-found)))

  ILookup
  (-lookup [this k]
    (-lookup this k nil))
  (-lookup [this k not-found]
    (let [cnt (or -cached-count
                  (set! -cached-count (.-count native)))]
      (if (and (number? k) (>= k 0) (< k (.call cnt native)))
        (let [nth-fn (or -cached-nth
                         (set! -cached-nth (.-nth native)))]
          (.call nth-fn native k))
        not-found)))

  ICollection
  (-conj [_ v]
    (let [conj-fn (or -cached-conj
                      (set! -cached-conj (.-conj native)))]
      (NativePersistentVector. (.call conj-fn native v) nil nil nil nil nil nil nil nil nil)))

  IEmptyableCollection
  (-empty [_]
    (let [empty-fn (or -cached-empty
                       (set! -cached-empty (.-empty native-factory)))]
      (NativePersistentVector. (.call empty-fn native-factory) nil nil nil nil nil nil nil nil nil)))

  IStack
  (-peek [_]
    (let [cnt (or -cached-count
                  (set! -cached-count (.-count native)))]
      (when (pos? (.call cnt native))
        (let [last-fn (or -cached-last
                          (set! -cached-last (.-last native)))]
          (.call last-fn native)))))
  (-pop [_]
    (let [pop-fn (or -cached-pop
                     (set! -cached-pop (.-pop native)))]
      (NativePersistentVector. (.call pop-fn native) nil nil nil nil nil nil nil nil nil)))

  ISeqable
  (-seq [this]
    (let [cnt (or -cached-count
                  (set! -cached-count (.-count native)))
          n (.call cnt native)]
      (when (pos? n)
        ;; Use native toArray for efficient seq conversion
        (let [to-arr (or -cached-toArray
                         (set! -cached-toArray (.-toArray native)))]
          (seq (.call to-arr native))))))

  ISequential

  IEquiv
  (-equiv [this other]
    (cond
      (instance? NativePersistentVector other)
      (let [to-arr (or -cached-toArray
                       (set! -cached-toArray (.-toArray native)))
            other-native (.-native other)
            other-to-arr (.-toArray other-native)]
        (= (.call to-arr native) (.call other-to-arr other-native)))

      (sequential? other)
      (= (seq this) (seq other))

      :else false))

  IHash
  (-hash [this]
    (hash (into [] this)))

  IReduce
  (-reduce [this f]
    (let [cnt (or -cached-count
                  (set! -cached-count (.-count native)))
          n (.call cnt native)]
      (if (zero? n)
        (f)
        ;; Use toArray for efficient iteration instead of repeated nth calls
        (let [to-arr (or -cached-toArray
                         (set! -cached-toArray (.-toArray native)))
              arr (.call to-arr native)]
          (loop [i 1
                 acc (aget arr 0)]
            (if (< i n)
              (let [result (f acc (aget arr i))]
                (if (reduced? result)
                  @result
                  (recur (inc i) result)))
              acc))))))
  (-reduce [this f init]
    (let [cnt (or -cached-count
                  (set! -cached-count (.-count native)))
          n (.call cnt native)]
      ;; Use toArray for efficient iteration
      (let [to-arr (or -cached-toArray
                       (set! -cached-toArray (.-toArray native)))
            arr (.call to-arr native)]
        (loop [i 0
               acc init]
          (if (< i n)
            (let [result (f acc (aget arr i))]
              (if (reduced? result)
                @result
                (recur (inc i) result)))
            acc)))))

  IAssociative
  (-assoc [_ k v]
    (let [cnt (or -cached-count
                  (set! -cached-count (.-count native)))]
      (if (and (number? k) (>= k 0) (< k (.call cnt native)))
        (let [assoc-fn (or -cached-assoc
                           (set! -cached-assoc (.-assoc native)))]
          (NativePersistentVector. (.call assoc-fn native k v) nil nil nil nil nil nil nil nil nil))
        (throw (js/Error. (str "Index " k " out of bounds for vector of size " (.call cnt native)))))))
  (-contains-key? [_ k]
    (let [cnt (or -cached-count
                  (set! -cached-count (.-count native)))]
      (and (number? k) (>= k 0) (< k (.call cnt native)))))

  IFind
  (-find [this k]
    (let [cnt (or -cached-count
                  (set! -cached-count (.-count native)))]
      (when (and (number? k) (>= k 0) (< k (.call cnt native)))
        (let [nth-fn (or -cached-nth
                         (set! -cached-nth (.-nth native)))]
          (cljs.core/MapEntry. k (.call nth-fn native k) nil)))))

  IVector
  (-assoc-n [_ n val]
    (let [cnt (or -cached-count
                  (set! -cached-count (.-count native)))]
      (if (and (>= n 0) (< n (.call cnt native)))
        (let [assoc-fn (or -cached-assoc
                           (set! -cached-assoc (.-assoc native)))]
          (NativePersistentVector. (.call assoc-fn native n val) nil nil nil nil nil nil nil nil nil))
        (throw (js/Error. (str "Index " n " out of bounds for vector of size " (.call cnt native)))))))

  IFn
  (-invoke [this k]
    (-nth this k))
  (-invoke [this k not-found]
    (-nth this k not-found))

  IPrintWithWriter
  (-pr-writer [this writer opts]
    (-write writer "#hermes/pvec ")
    (-write writer (str this))))

;; Extend first to use native .first() method for NativePersistentVector
;; This avoids the overhead of going through ISeqable
(extend-type NativePersistentVector
  INext
  (-next [this]
    (let [cnt (.-count (.-native this))
          n (.call cnt (.-native this))]
      (when (> n 1)
        ;; Return rest of the seq from toArray
        (let [to-arr (.-toArray (.-native this))
              arr (.call to-arr (.-native this))]
          (next (seq arr)))))))

;; Override first for NativePersistentVector to use native .first() directly
(defn native-first
  "Fast first for native vectors - uses native .first() method."
  [v]
  (when (instance? NativePersistentVector v)
    (let [native (.-native v)
          first-fn (.-first native)]
      (.call first-fn native))))

;; Factory functions

(defn empty-vector
  "Returns an empty native persistent vector."
  []
  (NativePersistentVector. (.empty native-factory) nil nil nil nil nil nil nil nil nil))

(defn from-array
  "Creates a native persistent vector from a JavaScript array."
  [arr]
  (NativePersistentVector. (.from native-factory arr) nil nil nil nil nil nil nil nil nil))

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

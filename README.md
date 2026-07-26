# matchline

A NASDAQ ITCH 5.0 feed handler and limit order book in C++20.

matchline parses ITCH 5.0 binary market data directly from a memory-mapped
file and reconstructs a limit order book from the stream of add, cancel,
execute, delete, and replace events. The order book comes in two forms: a
straightforward reference implementation built on standard containers, and
a fast implementation built from a flat order pool, intrusive linked lists,
a two-level bitset price index, and an open-addressed hash map. The two are
checked against each other continuously. So every operation the fast book
supports is run through both and compared, so the fast book's correctness
never has to be taken on faith.


## Design decisions

- **Strong types over raw integers.** `Price`, `Quantity`, and `OrderId`
  each wrap a single integer behind an `explicit` constructor with no
  implicit conversion. This is what stops a share count from being passed
  where a price belongs — enforced at compile time, at zero runtime cost.
- **Prices are integer ticks, never floating point.** ITCH prices are
  4-byte unsigned integers with four implied decimals; storing the raw
  integer keeps comparisons exact and deterministic across compilers. Floats are not precise in all cases and therefroe not suitable.
- **The parser takes its handler as a template parameter, not
  `std::function`.** This lets every `on_add`/`on_cancel`/etc. call be
  inlined by the compiler instead of going through an indirect call in the
  hottest loop in the project.
- **A slow, obviously-correct reference book exists specifically so the
  fast one doesn't have to be trusted by inspection.** `ReferenceBook`
  uses `std::map`/`std::list`/`std::unordered_map` — nothing about it
  requires more than a glance to believe it's right. It's the oracle the
  fast book is checked against, not a fallback path.
- **The fast book trades generality for speed, explicitly.** `OrderBook`
  maps prices onto a flat, fixed size array starting at a caller-chosen
  base price; a price outside that range throws rather than silently
  falling back to something slower. This is a real limitation, not hidden
  behind a default. Therefore the caller has to know roughly what range a symbol's
  day will trade in.


## ReferenceBook vs OrderBook

Same public interface, same observable behavior but differently written for their purpose:

| | ReferenceBook | OrderBook |
|---|---|---|
| Order lookup | `std::unordered_map<OrderId, Order>` | Open-addressed hash map, linear probing, no per-insert allocation |
| Price levels | `std::map<Price, std::list<OrderId>>` (tree) | Flat `std::vector<PriceLevel>`, indexed directly |
| Orders at a level | `std::list<OrderId>`, separately allocated nodes | Intrusive list threaded through the order's own `prev`/`next` fields |
| Best-price lookup | Tree traversal (`begin()`), O(log n) | Two-level bitset + `countl_zero`/`countr_zero`, O(1) |
| Cancel given an ID | Hash lookup, then linear scan of the level's list | Hash lookup gives the pool index directly; O(1) splice |
| Memory | Heap-allocated per order and per list node | One pre-sized flat array, no allocation after construction |
| Price range | Unbounded | Bounded at construction; out-of-range throws |


Built with the help of AI tools while learning low latency system design and modern C++.


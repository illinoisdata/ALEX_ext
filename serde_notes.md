# Plan to Serialize ALEX

## Checkpoint 1: Whole Dump

Serialize whole ALEX to file and deserialize successfully.
- Should work on a benchmark (say `fb_1M_uint64`)

`alex::Alex<T, P, Compare, Alloc, allow_duplicates>` (`src/core/alex.h`)
```c++
AlexNode<T, P>* root_node_ = nullptr;
AlexModelNode<...> superroot_ = nullptr;
struct Params {
  double expected_insert_frac = 1;
  int max_node_size = 1 << 24;
  bool approximate_model_computation = true;
  bool approximate_cost_computation = false;
} params_;
struct DerivedParams {
  int max_fanout = 1 << 21;
  int max_data_node_slots = (1 << 24) / sizeof(V);
} derived_params_;
struct Stats {
  int num_keys = 0;
  int num_model_nodes = 0;
  int num_data_nodes = 0;
  int num_expand_and_scales = 0;
  int num_expand_and_retrains = 0;
  int num_downward_splits = 0;
  int num_sideways_splits = 0;
  int num_model_node_expansions = 0;
  int num_model_node_splits = 0;
  long long num_downward_split_keys = 0;
  long long num_sideways_split_keys = 0;
  long long num_model_node_expansion_pointers = 0;
  long long num_model_node_split_pointers = 0;
  mutable long long num_node_lookups = 0;
  mutable long long num_lookups = 0;
  long long num_inserts = 0;
  double splitting_time = 0;
  double cost_computation_time = 0;
} stats_;
struct ExperimentalParams {
  int fanout_selection_method = 0;
  int splitting_policy_method = 1;
  bool allow_splitting_upwards = false;
} experimental_params_;
struct InternalStats {
  T key_domain_min_ = std::numeric_limits<T>::max();
  T key_domain_max_ = std::numeric_limits<T>::lowest();
  int num_keys_above_key_domain = 0;
  int num_keys_below_key_domain = 0;
  int num_keys_at_last_right_domain_resize = 0;
  int num_keys_at_last_left_domain_resize = 0;
} istats_;
Compare key_less_ = Compare();
Alloc allocator_ = Alloc();
```

`AlexNode<T, P>` (`src/core/alex_nodes.h`)
```c++
bool is_leaf_ = false;
uint8_t duplication_factor_ = 0;
short level_ = 0;
LinearModel<T> model_;
double cost_ = 0.0;
```

`AlexModelNode<T, P>` (`src/core/alex_nodes.h`)
```c++
// super --> AlexNode<T, P>
const Alloc& allocator_;
int num_children_ = 0;
AlexNode<T, P>** children_ = nullptr;  // Array pointer
```

`AlexDataNode<T, P>` (`src/core/alex_nodes.h`)
```c++
// super --> AlexNode<T, P>
const Compare& key_less_;
const Alloc& allocator_;
self_type* next_leaf_ = nullptr;  // AlexDataNode
self_type* prev_leaf_ = nullptr;  // AlexDataNode
#if ALEX_DATA_NODE_SEP_ARRAYS
  T* key_slots_ = nullptr;  // Array pointer
  P* payload_slots_ = nullptr;  // Array pointer
#else
  V* data_slots_ = nullptr;  // Array pointer
#endif
int data_capacity_ = 0;
int num_keys_ = 0;
uint64_t* bitmap_ = nullptr;  // Array pointer
int bitmap_size_ = 0;
double expansion_threshold_ = 1;
double contraction_threshold_ = 0;
int max_slots_ = kDefaultMaxDataNodeBytes_ / sizeof(V);
long long num_shifts_ = 0;
long long num_exp_search_iterations_ = 0;
int num_lookups_ = 0;
int num_inserts_ = 0;
int num_resizes_ = 0;
T max_key_ = std::numeric_limits<T>::lowest();
T min_key_ = std::numeric_limits<T>::max();
int num_right_out_of_bounds_inserts_ = 0;
int num_left_out_of_bounds_inserts_ = 0;
double expected_avg_exp_search_iterations_ = 0;
double expected_avg_shifts_ = 0;
```

`LinearModel<T>` (`src/core/alex_base.h`)
```c++
double a_ = 0;
double b_ = 0;
```

## Checkpoint 2: Lazy Chunk Load

- [x] Install pager in AlexDataNode
- [x] Allocate page (`key_slots_`, `payload_slots_`, `data_slots_`, `bitmap_`) on serialize + store offset
- [x] Load on offset address based on mmap address

Disable lazy loading by passing `nullptr` pager to `Alex`.


## Checkpoint 3: Lazy Node Load

- [ ] Serialize all node types (`AlexModelNode` and `AlexDataNode`) to byte array
- [ ] Allocate page for binary nodes + store offset in serialized `children_`
- [ ] (???) Lazily adjust `children_` and data chunks (`key_slots_`, `payload_slots_`, `data_slots_`, `bitmap_`) as Alex access the arrays

This change will be more involved because A) the accesses are spread over the codebase, and B) lazy offset adjustment has to be during lookup, after deserialization.


## Bug: Cousin Pointers

Currently we skip serializing cousin pointers altogether because A) it leads to stack overflow due to very deep recursion in large datasets, and B) our benchmark doesn't need cousin walking anyway.

We could fix this by connecting `AlexDataNode` together at the end of loading. `link_data_nodes` or `link_all_data_nodes` seems relevant (and might just work). We won't fix for now to be generous because of B).

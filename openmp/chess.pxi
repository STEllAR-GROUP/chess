#include "parallel_support.hpp"

component task_component {
    void cancel();
    score_t search(search_info inf);
}

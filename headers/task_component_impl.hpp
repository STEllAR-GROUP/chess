#include "task_component.hpp"

void task_component::server::cancel() {
}

score_t task_component::server::search(search_info inf) {
    return bad_min_score;
}

#include "parallel_support.hpp"

component task_component {
    void cancel();
    score_t search(search_info inf);
}
// vector<task_component> vt;
// while(true) {
//   task_component c = wait_any(vt)
//   if(c->score_invalid())
//   foreach(v in vt) {
//     v->cancel()
//     ..
//   }
// }

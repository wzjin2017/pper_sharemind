import stdlib;
import shared3p;
import profiling;
import shared3p_sort;
import keydb;
import shared3p_keydb;
import oblivious;

domain pd_shared3p shared3p;

float EPSILON = 0.000000001;
uint64 MAX_UINT64 = 18446744073709551615;

template <domain D>
D bool jaccard(D uint64 inter, uint64 union, D float t) {
    // jaccard(x, y) = len(x & y) / len(x | y) 
    //               = len(x & y) / (len(x) + len(y) - len(x & y))
    D float32 jaccard_result = (float32)inter / ((float32)union - (float32)inter);
    return (jaccard_result + EPSILON) >= t;
}

template <domain D>
D bool jaccard_sort(D uint64 [[1]] a, D uint64 [[1]] b, D float32 t) {
    // both a and b can not have duplicated entries
    // a = [1, 2, 3], b = [3, 4]
    // c = [1, 2, 3, 3, 4]
    // c1 = [1, 2, 3, 3]
    // c2 = [2, 3, 3, 4]
    // intersection = sum(c1 == c2)
    D uint64 [[1]] c = cat(a, b);
    c = quicksort(c);
    D uint64 match_counter = sum(c[:size(c)-1] == c[1:]);
    return jaccard(match_counter, size(a) + size(b), t);
}

void main() {
    string a_prefix = argument("a_prefix");
    uint64 a_size = argument("a_size");
    string b_prefix = argument("b_prefix");
    uint64 b_size = argument("b_size");
    pd_shared3p float32 t = argument("t");
    pd_shared3p uint64 [[1]] result (0);
    
    keydb_connect("dbhost");
    
    for (uint64 i = 0; i < a_size; i++) {
        for (uint64 j = 0; j < b_size; j++) {
            string a_key = a_prefix + tostring(i);
            string b_key = b_prefix + tostring(j);
            pd_shared3p uint64 [[1]] a = keydb_get(a_key);
            pd_shared3p uint64 [[1]] b = keydb_get(b_key);

            pd_shared3p bool jaccard_result = jaccard_sort(a, b, t);

            pd_shared3p uint64 [[1]] pair_id (2);
            pair_id[0] = i;
            pair_id[1] = j;
            pd_shared3p uint64 [[1]] not_pair_id (2) = MAX_UINT64;
            result = cat(result, choose(jaccard_result, pair_id, not_pair_id));
        }
    }
    
    keydb_disconnect();

    publish("result", result);
}

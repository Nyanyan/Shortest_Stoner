#include <algorithm>
#include "board.hpp"

using namespace std;

#define ESCAPE_INF 100

#define ESCAPE_MAX_PATH_LENGTH 15

struct Stoner_solution{
    vector<int> path;
    int escape_length;
};

string convert_path(vector<int> path){
    string res;
    for (int coord: path){
        int rev_coord = HW2_M1 - coord;
        res += (char)(rev_coord % HW + 'a');
        res += (char)(rev_coord / HW + '1');
    }
    return res;
}

bool cmp_shorter_stoner(Stoner_solution &a, Stoner_solution &b){
    if (a.path.size() != b.path.size())
        return a.path.size() < b.path.size();
    return a.escape_length < b.escape_length;
}

bool satisfy_all(uint64_t x, uint64_t mask){
    return (x & mask) == mask;
}

bool satisfy_at_least_one(uint64_t x, uint64_t mask){
    return (x & mask) > 0;
}

bool cannot_put_least_one(Board board, uint64_t mask){
    return (board.get_legal() & mask) == 0ULL;
}

bool cannot_put_least_one(uint64_t legal, uint64_t mask){
    return (legal & mask) == 0ULL;
}

bool is_stoner3_one_direction(Board board){
    bool res = true;
    res &= !satisfy_at_least_one(board.player | board.opponent, 0x00000000000002F1ULL); // empty shape
    res &= satisfy_all(board.player, 0x000000000080000EULL); // escaper on a6, e8, f8, g8
    res &= satisfy_all(board.opponent, 0x0000000000004000ULL); // attacker on b7
    res &= cannot_put_least_one(board, 0x0000000000000080ULL); // escaper cannot put on a8 now
    return res;
}

bool is_stoner4_one_direction(Board board){
    bool res = true;
    res &= !satisfy_at_least_one(board.player | board.opponent, 0x00000000000002E1ULL); // empty shape
    res &= satisfy_all(board.player, 0x000000000000001EULL); // escaper on d8, e8, f8, g8
    res &= satisfy_all(board.opponent, 0x0000000000004000ULL); // attacker on b7
    res &= cannot_put_least_one(board, 0x0000000000000080ULL); // escaper cannot put on a8 now
    res &= !satisfy_at_least_one(board.player | board.opponent, 0x0000000000000081ULL); // corner stability
    res &= satisfy_at_least_one(board.player, 0x4040404040400000ULL); // escaper's disc is on B column // not for reserved-stoner
    return res;
}

bool is_stoner3_1_one_direction(Board board){
    bool res = true;
    res &= !satisfy_at_least_one(board.player | board.opponent, 0x00000000000002D1ULL); // empty shape
    res &= satisfy_all(board.player, 0x000000000000000EULL); // escaper on e8, f8, g8
    res &= satisfy_all(board.opponent, 0x0000000000004020ULL); // attacker on b7, c8
    res &= cannot_put_least_one(board, 0x0000000000000080ULL); // escaper cannot put on a8 now
    res &= !satisfy_at_least_one(board.player | board.opponent, 0x0000000000000081ULL); // corner stability
    res &= satisfy_at_least_one(board.player, 0x4040404040400000ULL); // escaper's disc is on B column // not for reserved-stoner
    return res;
}

bool is_stoner(Board board){
    return
        is_stoner3_one_direction(board) | 
        is_stoner4_one_direction(board) | 
        is_stoner3_1_one_direction(board);
}

bool is_stoner_success(Board board){
    // escaper's move
    uint64_t legal = board.get_legal();
    if (satisfy_all(board.player, 0x000000000000001EULL) || 
        ((board.opponent & 0x0000000000000020ULL) > 0)){ // stoner type-4 or type-3-1
        if (legal & 0x0000000000000040ULL){ // b8 puttable?
            Flip e_flip;
            e_flip.calc_flip(board.player, board.opponent, 6); // put on b8
            board.move(&e_flip);
                if (cannot_put_least_one(board, 0x0000000000000080ULL)) // attacker cannot put a8?
                    return false;
            board.undo(&e_flip);
        }
    } else{ // stoner type-3
        if (legal & 0x0000000000000020ULL){ // c8 puttable?
            Flip e_flip;
            e_flip.calc_flip(board.player, board.opponent, 5); // put on c8
            board.move(&e_flip);
                if (cannot_put_least_one(board, 0x0000000000000080ULL)) // attacker cannot put a8?
                    return false;
            board.undo(&e_flip);
        }
    }
    return true;
}

int is_escapable_escaper(Board board, int depth, bool find_shortest_path);

int is_escapable_attacker(Board board, int depth, bool find_shortest_path){
    if (depth > ESCAPE_MAX_PATH_LENGTH)
        return ESCAPE_INF; // escaped
    // attacker's move
    //     try to find at least one way to execute stoner.

    // 1. special case
    //     a. attacker can put on a8 or h8, then stoner success
    //     b. execute stoner with putting disc on next to escaper's edge
    // if escapable:
    // 2. if escaper can put on a8 in the next turn, then attacker have to 
    //     a. re-build the X line
    // 3. else if attacker cannot execute stoner now
    //     a. put disc and make it executable
    //     b. others
    // 4. else
    //     a. put everywhere
    int shortest_stoner_escape = ESCAPE_INF;
    uint64_t a_legal = board.get_legal();
    if (a_legal == 0){ // passed
        board.pass();
        if (board.get_legal())
            return is_escapable_escaper(board, depth, find_shortest_path);
        return ESCAPE_INF; // both passed: attack failed
    }
    uint_fast8_t a_cell;
    Flip a_flip;
    // 1
    if (a_legal & 0x0000000000000081ULL) // 1-a
        return 0;
    uint64_t execute_stoner_puttable = 0;
    uint_fast8_t execute_stoner_place = 64;
    if (satisfy_all(board.opponent, 0x000000000000001EULL)){ // stoner type-4
        execute_stoner_puttable = 0x0000000000000020ULL;
        execute_stoner_place = 5;
    } else{ // stoner type-3 or type-3-1
        execute_stoner_puttable = 0x0000000000000010ULL;
        execute_stoner_place = 4;
    }
    if (a_legal & execute_stoner_puttable){ // 1-b
        a_flip.calc_flip(board.player,board.opponent, execute_stoner_place);
        board.move(&a_flip);
            if (is_stoner_success(board))
                return 0;
        board.undo(&a_flip);
        a_legal ^= execute_stoner_puttable;
    }
    if (!cannot_put_least_one(calc_legal(board.opponent, board.player), 0x0000000000000080ULL)){ // 2
        for (a_cell = first_bit(&a_legal); a_legal; a_cell = next_bit(&a_legal)){ // 2-a
            a_flip.calc_flip(board.player,board.opponent, a_cell);
            board.move(&a_flip);
                if (cannot_put_least_one(board, 0x0000000000000080ULL)){ // escaper cannot put on a8?
                    int n_res = is_escapable_escaper(board, depth + 1, find_shortest_path);
                    if (n_res < ESCAPE_INF){
                        if (find_shortest_path)
                            shortest_stoner_escape = min(shortest_stoner_escape, n_res + 2);
                        else
                            return n_res + 2;
                    }
                }
            board.undo(&a_flip);
        }
    } else if ((a_legal & execute_stoner_puttable) == 0){ // 3
        uint64_t a_passive_legal = 0;
        for (a_cell = first_bit(&a_legal); a_legal; a_cell = next_bit(&a_legal)){ // 3-a
            a_flip.calc_flip(board.player,board.opponent, a_cell);
            board.move(&a_flip);
                if (!cannot_put_least_one(calc_legal(board.opponent, board.player), execute_stoner_puttable)){
                    int n_res = is_escapable_escaper(board, depth + 1, find_shortest_path);
                    if (n_res < ESCAPE_INF){
                        if (find_shortest_path)
                            shortest_stoner_escape = min(shortest_stoner_escape, n_res + 2);
                        else
                            return n_res + 2;
                    }
                } else
                    a_passive_legal ^= 1ULL << a_cell;
            board.undo(&a_flip);
        }
        for (a_cell = first_bit(&a_passive_legal); a_passive_legal; a_cell = next_bit(&a_passive_legal)){ // 3-b
            a_flip.calc_flip(board.player,board.opponent, a_cell);
            board.move(&a_flip);
                int n_res = is_escapable_escaper(board, depth + 1, find_shortest_path);
                if (n_res < ESCAPE_INF){
                    if (find_shortest_path)
                            shortest_stoner_escape = min(shortest_stoner_escape, n_res + 2);
                        else
                            return n_res + 2;
                }
            board.undo(&a_flip);
        }
    } else{ // 4
        for (a_cell = first_bit(&a_legal); a_legal; a_cell = next_bit(&a_legal)){
            a_flip.calc_flip(board.player,board.opponent, a_cell);
            board.move(&a_flip);
                int n_res = is_escapable_escaper(board, depth + 1, find_shortest_path);
                if (n_res < ESCAPE_INF){
                    if (find_shortest_path)
                            shortest_stoner_escape = min(shortest_stoner_escape, n_res + 2);
                        else
                            return n_res + 2;
                }
            board.undo(&a_flip);
        }
    }
    return shortest_stoner_escape;
}

int is_escapable_escaper(Board board, int depth, bool find_shortest_path){
    if (depth > ESCAPE_MAX_PATH_LENGTH)
        return ESCAPE_INF; // escaped
    // escaper's move
    //     try to find a escapable way. if at least one way found, then you can escape.

    // 1. if escaper can put on a8 or h8
    //     a. put there and escaper can escape
    // if not escapable:
    // 2. if attacker can execute stoner with putting disc on next to escaper's edge
    //     a. both disturb it and cut the line
    //     b. only disturb it
    //     c. others
    // 3. else 
    //     a. cut the X line
    //     b. others
    Flip e_flip;
    uint64_t e_legal = board.get_legal();
    if (e_legal == 0){ // passed
        board.pass();
        if (board.get_legal())
            return is_escapable_attacker(board, depth, find_shortest_path);
        return ESCAPE_INF; // both passed: escaped!
    }
    uint_fast8_t e_cell;
    if (e_legal & 0x0000000000000081ULL) // 1
        return ESCAPE_INF; // escaped!
    int longest_stoner_escape = 0;
    uint64_t execute_stoner_puttable = 0ULL;
    if (satisfy_all(board.opponent, 0x000000000000001EULL)) // stoner type-4
        execute_stoner_puttable = 0x0000000000000020ULL;
    else
        execute_stoner_puttable = 0x0000000000000010ULL;
    if (calc_legal(board.opponent, board.player) & execute_stoner_puttable){ // 2
        uint64_t e_passive_legal = 0;
        for (e_cell = first_bit(&e_legal); e_legal; e_cell = next_bit(&e_legal)){ // 2-a
            e_flip.calc_flip(board.player, board.opponent, e_cell);
            board.move(&e_flip);
                if (cannot_put_least_one(board, execute_stoner_puttable) && !cannot_put_least_one(calc_legal(board.opponent, board.player), 0x0000000000000080ULL)){
                    int stoner_escape = is_escapable_attacker(board, depth + 1, find_shortest_path);
                    if (stoner_escape < ESCAPE_INF){
                        longest_stoner_escape = max(longest_stoner_escape, stoner_escape);
                    } else
                        return ESCAPE_INF; // escaped!
                } else
                    e_passive_legal ^= 1ULL << e_cell;
            board.undo(&e_flip);
        }
        uint64_t e_passive_legal2 = 0;
        for (e_cell = first_bit(&e_passive_legal); e_passive_legal; e_cell = next_bit(&e_passive_legal)){ // 2-b
            e_flip.calc_flip(board.player, board.opponent, e_cell);
            board.move(&e_flip);
                if (cannot_put_least_one(board, execute_stoner_puttable)){
                    int stoner_escape = is_escapable_attacker(board, depth + 1, find_shortest_path);
                    if (stoner_escape < ESCAPE_INF){
                        longest_stoner_escape = max(longest_stoner_escape, stoner_escape);
                    } else
                        return ESCAPE_INF; // escaped!
                } else
                    e_passive_legal2 ^= 1ULL << e_cell;
            board.undo(&e_flip);
        }
        for (e_cell = first_bit(&e_passive_legal2); e_passive_legal2; e_cell = next_bit(&e_passive_legal2)){ // 2-c
            e_flip.calc_flip(board.player, board.opponent, e_cell);
            board.move(&e_flip);
                int stoner_escape = is_escapable_attacker(board, depth + 1, find_shortest_path);
                if (stoner_escape < ESCAPE_INF){
                    longest_stoner_escape = max(longest_stoner_escape, stoner_escape);
                } else
                    return ESCAPE_INF; // escaped!
            board.undo(&e_flip);
        }
    } else{ // 3
        uint64_t e_passive_legal = 0;
        for (e_cell = first_bit(&e_legal); e_legal; e_cell = next_bit(&e_legal)){ // 3-a
            e_flip.calc_flip(board.player, board.opponent, e_cell);
            board.move(&e_flip);
                if (!cannot_put_least_one(calc_legal(board.opponent, board.player), 0x0000000000000080ULL)){
                    int stoner_escape = is_escapable_attacker(board, depth + 1, find_shortest_path);
                    if (stoner_escape < ESCAPE_INF){
                        longest_stoner_escape = max(longest_stoner_escape, stoner_escape);
                    } else
                        return ESCAPE_INF; // escaped!
                } else
                    e_passive_legal ^= 1ULL << e_cell;
            board.undo(&e_flip);
        }
        for (e_cell = first_bit(&e_passive_legal); e_passive_legal; e_cell = next_bit(&e_passive_legal)){ // 3-b
            e_flip.calc_flip(board.player, board.opponent, e_cell);
            board.move(&e_flip);
                int stoner_escape = is_escapable_attacker(board, depth + 1, find_shortest_path);
                if (stoner_escape < ESCAPE_INF){
                    longest_stoner_escape = max(longest_stoner_escape, stoner_escape);
                } else
                    return ESCAPE_INF; // escaped!
            board.undo(&e_flip);
        }
    }
    return longest_stoner_escape;
}

int n_stoner_like_solutions, n_stoner_solutions;

vector<Stoner_solution> find_stoners(Board board, int depth, vector<int> &path){
    vector<Stoner_solution> res;
    if (depth == 0){
        if (is_stoner(board)){
            Stoner_solution elem;
            for (int policy: path)
                elem.path.emplace_back(policy);
            //elem.escape_length = ESCAPE_INF;
            //res.emplace_back(elem);
            ++n_stoner_like_solutions;
            cerr << n_stoner_like_solutions << "th stoner-like path found " << convert_path(elem.path);
            elem.escape_length = is_escapable_escaper(board, 0, false);
            if (elem.escape_length < ESCAPE_INF)
                elem.escape_length = is_escapable_escaper(board, 0, true);
            cerr << "\r                                                                                ";
            cerr << "\r";
            if (elem.escape_length < ESCAPE_INF){
                ++n_stoner_solutions;
                cerr << "\r" << n_stoner_solutions << "th stoner found " << convert_path(elem.path) << " shortest escape path " << elem.escape_length << " moves" << endl;
                res.emplace_back(elem);
            }
        }
        return res;
    }
    uint64_t discs = board.player | board.opponent;
    bool can_be_a_stoner = false;
    if (depth >= 6 - pop_count_ull(discs & 0x000000000040401EULL))
        can_be_a_stoner = true;
    else if (depth >= 5 - pop_count_ull(discs & 0x000000000080400EULL)) // type-3
        can_be_a_stoner = true;
    else if (depth >= 6 - pop_count_ull(discs & 0x000000000040402EULL)) // type-3-1
        can_be_a_stoner = true;
    if (!can_be_a_stoner)
        return res;
    uint64_t legal = board.get_legal();
    legal &= ~0x00000000000000C1ULL; // these moves do not lead to a stoner.
    if (legal){ // if not pseudo-passed
        Flip flip;
        for (uint_fast8_t cell = first_bit(&legal); legal; cell = next_bit(&legal)){
            flip.calc_flip(board.player, board.opponent, cell);
            path.emplace_back(cell);
            board.move(&flip);
                vector<Stoner_solution> n_res = find_stoners(board, depth - 1, path);
                for (Stoner_solution solution: n_res)
                    res.emplace_back(solution);
            board.undo(&flip);
            path.pop_back();
        }
    }
    return res;
}

vector<int> mirror_path(vector<int> path){
    vector<int> res;
    for (int coord: path){
        int x = coord % HW;
        int y = coord / HW;
        res.emplace_back(HW_M1 - x + y * HW);
    }
    return res;
}

vector<Stoner_solution> find_stoners_root(int depth){
    Board board;
    board.player = 0x0000000810000000ULL;
    board.opponent = 0x0000001008000000ULL;
    vector<int> path;
    vector<Stoner_solution> res = find_stoners(board, depth, path);

    board.player = 0x0000001008000000ULL;
    board.opponent = 0x0000001008000000ULL;
    path.clear();
    vector<Stoner_solution> res_mirrored_start = find_stoners(board, depth, path);

    for (Stoner_solution mirrored_solution: res_mirrored_start){
        Stoner_solution solution;
        solution.path = mirror_path(mirrored_solution.path);
        solution.escape_length = mirrored_solution.escape_length;
        res.emplace_back(solution);
    }
    return res;
}

int main(){
    bit_init();
    flip_init();
    /*
    Board board;
    // not escapable
    //board.player = 0x0000000008D8080EULL;
    //board.opponent = 0x0000041C14244000ULL;
    board.player = 0x0000004000D0080EULL;
    board.opponent = 0x00000818382C4400ULL;

    // escapable
    //board.player = 0x0000000040C0100EULL;
    //board.opponent = 0x0000003838384800ULL;
    //board.player = 0x0000000048C0100EULL;
    //board.opponent = 0x0000003830304800ULL;

    board.print();
    cout << "is_stoner " << is_stoner(board) << endl;
    cout << is_escapable_escaper(board, 0) << endl;
    */

    for (int depth = 13; depth <= 16; ++depth){
        n_stoner_like_solutions = 0;
        n_stoner_solutions = 0;
        uint64_t strt = tim();
        vector<Stoner_solution> stoners = find_stoners_root(depth);
        if (stoners.size()){
            cout << stoners.size() << " solutions found at depth " << depth << " in " << tim() - strt << " ms" << endl;
            sort(stoners.begin(), stoners.end(), cmp_shorter_stoner);
            for (Stoner_solution sol: stoners){
                cout << convert_path(sol.path) << " " << sol.escape_length << endl;
            }
            break;
        } else{
            cout << "no stoner path found at depth " << depth << " in " << tim() - strt << " ms" << endl;
        }
    }
    
    return 0;
}
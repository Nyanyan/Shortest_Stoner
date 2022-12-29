#include "board.hpp"

using namespace std;

#define ESCAPE_INF 100

struct Stoner_solution{
    vector<int> path;
    int escape_length;
};

string convert_path(vector<int> path){
    string res;
    for (int coord: path){
        res += (char)(coord % HW + 'a');
        res += (char)(coord / HW + '1');
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
    res &= !satisfy_at_least_one(board.player | board.opponent, 0x00000000000000F1ULL); // edge shape
    res &= satisfy_all(board.player, 0x000000000080000EULL); // escaper on a6, e8, f8, g8
    res &= satisfy_all(board.opponent, 0x0000000000004000ULL); // attacker on b7
    res &= cannot_put_least_one(board, 0x0000000000000080ULL); // escaper cannot put on a8 now
    return res;
}

bool is_stoner4_one_direction(Board board){
    bool res = true;
    res &= !satisfy_at_least_one(board.player | board.opponent, 0x00000000000000E1ULL); // edge shape
    res &= satisfy_all(board.player, 0x000000000000001EULL); // escaper on d8, e8, f8, g8
    res &= satisfy_all(board.opponent, 0x0000000000004000ULL); // attacker on b7
    res &= cannot_put_least_one(board, 0x0000000000000080ULL); // escaper cannot put on a8 now
    res &= !satisfy_at_least_one(board.player | board.opponent, 0x0000000000000081ULL); // corner stability
    return res;
}

bool is_stoner3_1_one_direction(Board board){
    bool res = true;
    res &= !satisfy_at_least_one(board.player | board.opponent, 0x00000000000000D1ULL); // edge shape
    res &= satisfy_all(board.player, 0x000000000000000EULL); // escaper on e8, f8, g8
    res &= satisfy_all(board.opponent, 0x0000000000004020ULL); // attacker on b7, c8
    res &= cannot_put_least_one(board, 0x0000000000000080ULL); // escaper cannot put on a8 now
    res &= !satisfy_at_least_one(board.player | board.opponent, 0x0000000000000081ULL); // corner stability
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

int is_escapable_escaper(Board board);

int is_escapable_attacker(Board board){
    // attacker's move
    //     try to find at least one way to execute stoner.

    // 1. special case
    //     a. execute stoner with putting disc on next to escaper's edge
    // if escapable:
    // 2. if escaper can put on a8 in the next turn, then attacker have to 
    //     a. re-build the X line
    // 3. else if attacker cannot execute stoner now
    //     a. put disc and make it executable
    //     b. put everywhere
    // 4. else
    //     a. put everywhere
    int shortest_stoner_escape = ESCAPE_INF;
    uint64_t a_legal = board.get_legal();
    uint_fast8_t a_cell;
    Flip a_flip;
    // 1
    uint64_t execute_stoner_puttable = 0;
    uint_fast8_t execute_stoner_place = 64;
    if (satisfy_all(board.opponent, 0x000000000000001EULL)){ // stoner type-4
        execute_stoner_puttable = 0x0000000000000020ULL;
        execute_stoner_place = 5;
    } else{ // stoner type-3 or type-3-1
        execute_stoner_puttable = 0x0000000000000010ULL;
        execute_stoner_place = 4;
    }
    if (a_legal & execute_stoner_puttable){ // can execute stoner?
        a_flip.calc_flip(board.player,board.opponent, execute_stoner_place); // put on c8
        board.move(&a_flip);
            if (is_stoner_success(board))
                return 0;
        board.undo(&a_flip);
        a_legal ^= execute_stoner_puttable;
    }
    if (!cannot_put_least_one(calc_legal(board.opponent, board.player), 0x0000000000000080ULL)){ // 2
        for (a_cell = first_bit(&a_legal); a_legal; a_cell = next_bit(&a_legal)){
            a_flip.calc_flip(board.player,board.opponent, a_cell);
            board.move(&a_flip);
                if (cannot_put_least_one(board, 0x0000000000000080ULL)){ // escaper cannot put on a8
                    int n_res = is_escapable_escaper(board);
                    if (n_res < ESCAPE_INF){
                        return n_res + 2;
                        //shortest_stoner_escape = min(shortest_stoner_escape, n_res + 2);
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
                    int n_res = is_escapable_escaper(board);
                    if (n_res < ESCAPE_INF){
                        return n_res + 2;
                        //shortest_stoner_escape = min(shortest_stoner_escape, n_res + 2);
                    }
                } else
                    a_passive_legal ^= 1ULL << a_cell;
            board.undo(&a_flip);
        }
        for (a_cell = first_bit(&a_passive_legal); a_passive_legal; a_cell = next_bit(&a_passive_legal)){ // 3-b
            a_flip.calc_flip(board.player,board.opponent, a_cell);
            board.move(&a_flip);
                int n_res = is_escapable_escaper(board);
                if (n_res < ESCAPE_INF){
                    return n_res + 2;
                    //shortest_stoner_escape = min(shortest_stoner_escape, n_res + 2);
                }
            board.undo(&a_flip);
        }
    } else{ // 4
        for (a_cell = first_bit(&a_legal); a_legal; a_cell = next_bit(&a_legal)){
            a_flip.calc_flip(board.player,board.opponent, a_cell);
            board.move(&a_flip);
                int n_res = is_escapable_escaper(board);
                if (n_res < ESCAPE_INF){
                    return n_res + 2;
                    //shortest_stoner_escape = min(shortest_stoner_escape, n_res + 2);
                }
            board.undo(&a_flip);
        }
    }
    return shortest_stoner_escape;
}

int is_escapable_escaper(Board board){
    // escaper's move
    //     try to find a escapable way. if at least one way found, then you can escape.

    // 1. if escaper can put on a8 or h8
    //     a. put there and escaper can escape
    // if not escapable:
    // 2. if attacker can execute stoner with putting disc on next to escaper's edge
    //     a. disturb it
    //     b. put everywhere
    // 3. else 
    //     a. cut the X line
    //     b. put everywhere
    Flip e_flip;
    uint64_t e_legal = board.get_legal();
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
                if ((board.get_legal() & execute_stoner_puttable) == 0ULL){
                    int stoner_escape = is_escapable_attacker(board);
                    if (stoner_escape < ESCAPE_INF){
                        longest_stoner_escape = max(longest_stoner_escape, stoner_escape);
                    } else
                        return ESCAPE_INF; // escaped!
                } else
                    e_passive_legal ^= 1ULL << e_cell;
            board.undo(&e_flip);
        }
        for (e_cell = first_bit(&e_passive_legal); e_passive_legal; e_cell = next_bit(&e_passive_legal)){ // 2-b
            e_flip.calc_flip(board.player, board.opponent, e_cell);
            board.move(&e_flip);
                int stoner_escape = is_escapable_attacker(board);
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
                    int stoner_escape = is_escapable_attacker(board);
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
                int stoner_escape = is_escapable_attacker(board);
                if (stoner_escape < ESCAPE_INF){
                    longest_stoner_escape = max(longest_stoner_escape, stoner_escape);
                } else
                    return ESCAPE_INF; // escaped!
            board.undo(&e_flip);
        }
    }
    return longest_stoner_escape;
}

vector<Stoner_solution> find_stoners(Board board, int depth, vector<int> path){
    vector<Stoner_solution> res;
    if (depth == 0){
        if (is_stoner(board)){
            Stoner_solution elem;
            for (int policy: path)
                elem.path.emplace_back(policy);
            elem.escape_length = is_escapable_escaper(board);
            if (elem.escape_length < ESCAPE_INF)
                res.emplace_back(elem);
        }
        return res;
    }
    uint64_t legal = board.get_legal();
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
    Board board;
    // not escapable
    //board.player = 0x0000000008D8080EULL;
    //board.opponent = 0x0000041C14244000ULL;

    // escapable
    //board.player = 0x0000000040C0100EULL;
    //board.opponent = 0x0000003838384800ULL;

    board.print();
    cout << "is_stoner " << is_stoner(board) << endl;
    cout << is_escapable_escaper(board) << endl;
    /*
    for (int depth = 0; depth <= 16; ++depth){
        vector<Stoner_solution> stoners = find_stoners_root(depth);
        if (stoners.size()){
            cout << stoners.size() << " solutions found at depth " << depth << endl;
            sort(stoners.begin(), stoners.end(), cmp_shorter_stoner);
            for (Stoner_solution sol: stoners){
                cout << convert_path(sol.path) << " " << convert_path(sol.escape_path) << endl;
            }
            break;
        } else{
            cout << "no stoner path found at depth " << depth << endl;
        }
    }
    */
    return 0;
}
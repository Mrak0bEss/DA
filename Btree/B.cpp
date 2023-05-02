#include <iostream>
#include <cstring>
#include <fstream>
#include <time.h>

using namespace std;


struct Pair;
struct Data;
class List;
class TreeNode;
class BTree;

//само значение каждого элемента с которым работаем
struct Pair {
    char * key;
    unsigned long long value;
};

//вид в котором элементы хранятся (с ссылочкой на потомка)
struct Data {
    Pair * pair;
    TreeNode * child;
};

//список всех данных на одной ветке
struct List {
    Data data;
    List * next = nullptr;
    List * prev = nullptr;

    List(Pair * pair, TreeNode * child, List * next, List * prev) {
        this->data = {pair, child};
        this->family(next);
        if(prev) prev->family(this);
    }

    List(Pair * pair, TreeNode * child) : List(pair, child, nullptr, nullptr) {}

    List(Pair * pair) : List(pair, nullptr) {}

    bool is_terminal() {
        return this->data.pair == nullptr;
    }

    void family(List * other) {
        if(!other) return;
        this->next = other;
        other->prev = this;
    }

    void not_family() {
        if(!this->next) return;
        this->next->prev = nullptr;
        this->next = nullptr;
    }

    
};


class TreeNode {
private:
    int sz = 0;
    List * data = nullptr;

public:
    TreeNode(Pair * pair) {
        List * terminal = new List(nullptr);
        this->data = new List(pair);
        this->data->family(terminal);
        this->sz = 1;

    }

    TreeNode(List * data) {
        List * current = data;
        
        this->data = data;
        this->sz = 0;

        while(!current->is_terminal()) {
            this->sz++;
            current = current->next;
        }
    }

    bool is_leaf() {
        Data & head_data = this->data->data;
        return (head_data.pair) && (!head_data.child);
    }

    int get_sz() {
        return this->sz;
    }

    List * key_get(char * key) {
        List * current = this->data;

        while(!current->is_terminal()) {
            if(strcmp(current->data.pair->key, key) >= 0)
                break;
            current = current->next;
        }

        return current;
    }

    List * key_ind(int index) {
        List * current = this->data;

        while((index > 0) && (!current->is_terminal())) {
            current = current->next;
            index--;
        }

        return (index <= 0) ? current : nullptr;
    }

    TreeNode * split(List * current) {
        List * terminal = new List(nullptr, current->data.child);
        List * tail = current->prev;

        tail->not_family();

        if(tail) tail->family(terminal);
        else this->data = terminal; 

        TreeNode * right_part = new TreeNode(current);
        this->sz -= right_part->get_sz();

        return right_part;
    }

    void join(TreeNode * other) {
        List * terminal = this->data;
        List * tail;

        while(!terminal->is_terminal())
            terminal = terminal->next;

        tail = terminal->prev;

        if(!tail)
            this->data = other->data;
        else {
            tail->not_family();
            tail->family(other->data);
        }

        this->sz += other->sz;
        other->sz = 0;
        other->data = terminal;
    }

    bool contains(char * key) {
        List * found = this->key_get(key);

        if(found->is_terminal())
            return false;

        return strcmp(found->data.pair->key, key) == 0;
    }

    bool insert(List * new_node) {
        if(this->contains(new_node->data.pair->key))
            return false;

        List * next = this->key_get(new_node->data.pair->key);

        if(next->prev)
            next->prev->family(new_node);
        else
            this->data = new_node;

        new_node->family(next);
        this->sz++;

        return true;
    }

    void pop(List * popped_node) {
        List * prev_node = popped_node->prev;

        if(prev_node) {
            prev_node->not_family();
            prev_node->family(popped_node->next);
        } else {
            this->data = popped_node->next;
            popped_node->not_family();
        }

        this->sz--;
    }

    void debug() {
        List * current = data;

        while(current) {
            cout << "<" << current << ">";
            cout << "(child=" << current->data.child << ", ";
            cout << "prev=" << current->prev << ", next=" << current->next << "), ";
            current = current->next;
        }

        cout << endl;
    }

    void print() {
        cout << "(";

        List * current = data;
        while(current->data.pair) {
            cout << "('" << current->data.pair->key;
            cout << "', " <<  current->data.pair->value << "), ";
            current = current->next;
        }

        cout << ")" << endl;
    }


};


class BTree {
private:
    TreeNode * root;
    int t;

    bool __insert(TreeNode * curr, Pair * pair) {
        if(curr->is_leaf())
            return curr->insert(new List(pair));

        if(curr->contains(pair->key))
            return false;

        List * next = curr->key_get(pair->key);
        TreeNode * child = next->data.child;

        if(!this->__insert(child, pair))
            return false;

        if(child->get_sz() >= (2 * this->t - 1)) {
            TreeNode * mid = child->split(child->key_ind(this->t - 1));
            TreeNode * right = mid->split(mid->key_ind(1));
            List * mid_node = mid->key_ind(0);
            mid->pop(mid_node);

            mid_node->data.child = child;
            next->data.child = right;
            curr->insert(mid_node);

            delete mid;
        }

        return true;
    }

    List * __pop(TreeNode * curr, char * key) {
        if(!curr) {
            return nullptr;
        } 

        List * next_node;
        List * popped_node;

        if(curr->contains(key)) {
            if(curr->is_leaf()) {

                popped_node = curr->key_get(key);
                curr->pop(popped_node);
                return popped_node;
            }

            next_node = curr->key_get(key);
            TreeNode * child = next_node->data.child;
            popped_node = child->key_ind(child->get_sz() - 1);

            while(popped_node->next->data.child) {
                int last_index = popped_node->next->data.child->get_sz() - 1;
                popped_node = popped_node->next->data.child->key_ind(last_index);
            }

            popped_node = this->__pop(child, popped_node->data.pair->key);

            Pair * tmp = popped_node->data.pair;
            popped_node->data.pair = next_node->data.pair;
            next_node->data.pair = tmp;
        } else {
            
            next_node = curr->key_get(key);
            popped_node = this->__pop(next_node->data.child, key);
        }

        if(popped_node == nullptr)
            return nullptr;

        if(next_node->data.child->get_sz() < (this->t - 1)) {


            TreeNode * left_tree = (next_node->prev) ? next_node->prev->data.child : nullptr;
            TreeNode * right_tree = (next_node->next) ? next_node->next->data.child : nullptr;

            if(left_tree && left_tree->get_sz() >= t) {
                List * stolen_node = left_tree->key_ind(left_tree->get_sz() - 1);
                List * terminal = stolen_node->next;
                left_tree->pop(stolen_node);

                Pair * tmp = next_node->prev->data.pair;
                next_node->prev->data.pair = stolen_node->data.pair;
                stolen_node->data.pair = tmp;

                TreeNode * tmp2 = terminal->data.child;
                terminal->data.child = stolen_node->data.child;
                stolen_node->data.child = tmp2;

                next_node->data.child->insert(stolen_node);
                return popped_node;
            }

            if(right_tree && right_tree->get_sz() >= t) {
                List * stolen_node = right_tree->key_ind(0);
                right_tree->pop(stolen_node);

                Pair * tmp = next_node->data.pair;
                next_node->data.pair = stolen_node->data.pair;
                stolen_node->data.pair = tmp;

                next_node->data.child->insert(stolen_node);

                TreeNode * tmp2 = stolen_node->data.child;
                stolen_node->data.child = stolen_node->next->data.child;
                stolen_node->next->data.child = tmp2;

                return popped_node;
            }

            if(left_tree) {
                List * neighbor = next_node->prev;
                
                TreeNode * left_part = neighbor->data.child;
                TreeNode * right_part = next_node->data.child;
                TreeNode * mid = new TreeNode(neighbor->data.pair);
            

                mid->key_ind(0)->data.child = left_part->key_ind(left_part->get_sz())->data.child;
                left_part->join(mid);
                left_part->join(right_part);

                next_node->data.child = left_part;
                curr->pop(neighbor);

                delete neighbor;
                delete right_part;
                delete mid;

                return popped_node;
            }
            
            else {
                List * neighbor = next_node->next;
            
                TreeNode * left_part = next_node->data.child;
                TreeNode * right_part = neighbor->data.child;
                TreeNode * mid = new TreeNode(next_node->data.pair);
        
                mid->key_ind(0)->data.child = left_part->key_ind(left_part->get_sz())->data.child;
                left_part->join(mid);
                left_part->join(right_part);
                neighbor->data.child = left_part;
                curr->pop(next_node);

                
            }
        }

        return popped_node;
    }

    void __print(TreeNode * curr, int layer) {
        if(!curr) return;

        cout << layer << " : ";
        curr->print();

        for(int i = 0; i < curr->get_sz() + 1; i++)
            this->__print(curr->key_ind(i)->data.child, layer + 1);
    }

    void __debug(TreeNode * curr, int layer) {
        if(!curr) return;
        cout << layer << " : " << curr << " ";
        curr->debug();

        for(int i = 0; i < curr->get_sz() + 1; i++)
            this->__debug(curr->key_ind(i)->data.child, layer + 1);
    }

    void __clear(TreeNode * curr) {
        if(curr == nullptr) return;

        List * current = curr->key_ind(0);

        while(!current) {
            if(current->data.pair) delete current->data.pair;
            this->__clear(current->data.child);
        }

        
    }
public:
    BTree(int t) {
        this->root = nullptr;
        this->t = t;
    }

    int get_t() {return this->t;}
    TreeNode * & get_root() {return this->root;}

    bool insert(Pair * pair) {
        if(!this->root) {
            this->root = new TreeNode(pair);
            return true;
        }

        if(!this->__insert(this->root, pair))
            return false;

        if(this->root->get_sz() >= (2 * this->t - 1)) {
            TreeNode * mid = this->root->split(this->root->key_ind(this->t - 1));
            TreeNode * right = mid->split(mid->key_ind(1));
            List * root_el = mid->key_ind(0);

            root_el->data.child = this->root;
            root_el->next->data.child = right;
            this->root = mid;
        }

        return true;
    }

    // tree-pop
    Pair * pop(char * key) {
        List * popped_node = this->__pop(this->root, key);
        Pair * popped_el;

        if(!popped_node)
            return nullptr;

        if(this->root && (this->root->get_sz() == 0)) {
            TreeNode * unused_node = this->root;
            this->root = this->root->key_ind(0)->data.child;
            
        }

        popped_el = popped_node->data.pair;
        
        return popped_el;
    }

    Pair * find(char * key) {
        TreeNode * current = this->root;

        while(current) {
            if(current->contains(key))
                return current->key_get(key)->data.pair;
            current = current->key_get(key)->data.child;
        }

        return nullptr;
    }

    void clear() {
        this->__clear(this->root);
        this->root = nullptr;
    }

    void print() {
        this->__print(this->root, 0);
    }

    void debug() {
        this->__debug(this->root, 0);
    }
};


class NotOpenedException : public exception {
public:
    NotOpenedException() {}
};


class WrongFormat : public exception {
public:
    WrongFormat() {}
};


class UnableToWrite : public exception {
public:
    UnableToWrite() {}
};

//ввод вывод с файла и ошибки с этим связанные
class BTreeFiler {
public:
    void to_file(Data & data, ofstream & file) {
        this->to_file(data.child, file);
        if(!(file << " ")) throw UnableToWrite();
        this->to_file(data.pair, file);
        // if(!(file << " ")) throw UnableToWrite();
    }

    void to_file(TreeNode * node, ofstream & file) {
        if(node) {
            List * current = node->key_ind(0);
            
            if(!(file << node->get_sz())) throw UnableToWrite();

            while(current) {
                if(!(file << " ")) throw UnableToWrite();
                this->to_file(current->data, file);
                current = current->next;
            }
        } else {
            if(!(file << "0")) throw UnableToWrite();
        }
    }

    void to_file(Pair * pair, ofstream & file) {
        if(!pair) return;
        if(!(file << pair->key << " " << pair->value << " "))
            throw UnableToWrite();
    }

    void to_file(BTree & tree, ofstream & file) {
        if(!(file << tree.get_t() << " ")) throw UnableToWrite();
        this->to_file(tree.get_root(), file);
    }

    void from_file(Data & data, ifstream & file) {
        this->from_file(data.child, file);
        this->from_file(data.pair, file);
    }

    void from_file(TreeNode * &node, ifstream & file) {
        int sz;
        
        if(!(file >> sz)) throw WrongFormat();

        // cout << "sz " << sz << endl;
        if(sz) {
            List * head = new List(nullptr);
            List * current = head;

            while(sz--) {
                from_file(current->data, file);
                current->next = new List(nullptr);
                current = current->next;
            }

            from_file(current->data.child, file);
            node = new TreeNode(head);
        } else {
            node = nullptr;
        }
    }

    void from_file(Pair * &pair, ifstream & file) {
        char buff[256];
        unsigned long long value;

        if(!(file >> buff >> value)) throw WrongFormat();
        pair = new Pair{.key = strdup(buff), .value=value};
    }

    void from_file(BTree & tree, ifstream & file) {
        int t;
        if(!(file >> t)) throw WrongFormat();

        BTree new_tree(t);
        // cout << tree.get_root() << " ";
        from_file(new_tree.get_root(), file);
        tree = new_tree;
        // cout << new_tree.get_root() << endl;
    }

    BTreeFiler() {}
};



class NullKeyException : public exception {};

//все в один регистр
void my_tolower(char * str) {
    for(int i = 0; i < strlen(str); i++)
        str[i] = tolower(str[i]);
}


char * strdup_safety(char * src_string) {
    char * new_string = strdup(src_string);
    if(!new_string) throw NullKeyException();
    return new_string;
}


int main() {
    BTree tree(3);
    char * command_type = new char[260];
    char * key = new char[260];
    char * filename = new char[260];
    unsigned long long value;
    clock_t start = clock();
    while((cin >> command_type)) {
        //cout<<command_type<<endl;;
        try {
            // add operation
            if(!strcmp(command_type, (char*) "+")) {
                cin >> key >> value;
                my_tolower(key);
                Pair * pair = new Pair{.key = strdup_safety(key), .value = value};
                
                //if(tree.insert(pair))
                    //cout << "OK" << endl;
               // else
                 //   cout << "Exist" << endl;
            }

            // remove operation
            else if(!strcmp(command_type, (char*)"-")) {
                cin >> key;
                my_tolower(key);
                Pair * popped_pair = tree.pop(key);

                if(popped_pair) {
                    delete popped_pair;
                    cout << "OK" << endl;
                } else
                    cout << "NoSuchWord" << endl;
            }

            // to_file / from_file operation
            else if(!strcmp(command_type, (char*)"!")) {
                cin >> key >> filename;

                if(!strcmp(key, (char*)"Save")) {
                    ofstream file(filename);
                    if(!file.is_open()) throw NotOpenedException();
                    BTreeFiler().to_file(tree, file);
                    file.close();
                    cout << "OK" << endl;
                }

                if(!strcmp(key, (char*)"Load")) {
                    ifstream file(filename);
                    tree.clear();
                    if(!file.is_open()) throw NotOpenedException();
                    BTreeFiler().from_file(tree, file);
                    file.close();
                    cout << "OK" << endl;
                }
            }
            else if(!strcmp(command_type, (char*)"&")) {
                break;
            }
            
            // get operation
            else {
                my_tolower(command_type);
                Pair * found = tree.find(command_type);

                if(found) cout << "OK: " << found->value << endl;
                else cout << "NoSuchWord" << endl;
            }

        } catch(const WrongFormat & e) {
            cout << "OK" << endl;
        } catch(const NotOpenedException & e) {
            cout << "OK" << endl;
        } catch(const UnableToWrite & e) {
            cout << "ERROR: unable to write" << endl;
        } catch(const NullKeyException & e) {
            cout << "ERROR: can't allocate string" << endl;
        } catch(const exception & e) {
            cout << "ERROR: " << e.what() << endl;
        }
    }
    ifstream file(filename);
    clock_t end = clock();
    BTreeFiler().to_file(tree, file);
    clock_t end2 = clock();
    file.close();
    cout<< double(end2 - end)/CLOCKS_PER_SEC;

    tree.clear();

    return 0;
}

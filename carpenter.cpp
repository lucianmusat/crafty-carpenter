#include <iostream>
#include <set>
#include <vector>
#include <algorithm>
#include <deque>
#include <sstream>
#include <stdexcept>
#include <limits>
#include <cassert>

/*! \brief Some convenience types that will be used.
 *
 *  A strong typedef will be better suited for Item, but this will do for now.
 *  Chose int64 because I assume we are on a x64 platform and also to avoid
 *  issues with unsigned nr arithmetic.
 */
using Item = int64_t;
using CabinetIter = std::deque<Item>::iterator;
constexpr Item kSentinelItem = std::numeric_limits<Item>::max();


/*! \brief A struct that describes the return type.
 *
 *  In Cpp17 we could use std::variant for this.
 *  Using kSentinelItem value to show cabinet_nr is not valid.
 */
struct Result {
    int64_t cabinet_nr;
    std::string status;

    explicit Result() : cabinet_nr(kSentinelItem) {}

    explicit Result(int64_t nr) : cabinet_nr(nr) {}

    explicit Result(const std::string &str) : cabinet_nr(kSentinelItem), status(str) {}
};

/*! \brief A class that describes a cabinet that holds Items.
 *
 *  Chose std::deque as the underlying container for it because
 *  it is the closest one as a mental model for the requirements.
 *  We can push an Item to the front and pop the back but has linear time search.
 *  If performance would have been a major concern I might have implemented it using
 *  std::list and cache the iterators.
 */
class Cabinet {
public:
    explicit Cabinet(int64_t size)
            : mSize(size) {}

    CabinetIter end() { return mStorage.end(); }

    CabinetIter find(Item item) {
        return std::find(mStorage.begin(), mStorage.end(), item);
    }

/*
*   \brief Add an Item to the cabinet.
 *   If it doesn't fit, pop the oldest and return it.
*/
    Item addItem(Item item) {
        if (hasSpace()) {
            mStorage.push_front(item);
            return Item(kSentinelItem);
        } else {
            const auto lastItem = mStorage.back();
            mStorage.pop_back();
            mStorage.push_front(item);
            return lastItem;
        }
    }

/*
*   \brief Get the Item from cabinet at a specific position.
*/
    Item pop(const CabinetIter &item_it) {
        assert(item_it != end());
        Item item = *item_it;
        mStorage.erase(item_it);
        return item;
    }

/*
*   \brief Remove the oldest Item from the cabinet and return it.
*/
    Item pop_back() {
        assert(!mStorage.empty());
        auto lastItem = mStorage.back();
        mStorage.pop_back();
        return lastItem;
    }

    [[nodiscard]] bool hasSpace() const noexcept { return mStorage.size() < mSize; }

    [[nodiscard]] bool empty() const noexcept { return mStorage.empty(); }

private:
    uint16_t mSize;
    std::deque<Item> mStorage;
};

/*! \brief A class that describes the workshop.
 *
 *  It holds a number of cabinets of different sizes and also the current workbench, which is
 *  a Cabinet of size 1, and Outside, which is a Cabinet of "infinite" size.
 */
class WorkShop {
public:
    explicit WorkShop(const std::vector<int64_t> &cabinetSizes)
            : mWorkbench(1), mOutside(kSentinelItem) {
        mCabinets.reserve(cabinetSizes.size());
        for (auto size: cabinetSizes) {
            mCabinets.emplace_back(size);
        }
    }

/*
 *   \brief Work on an Item. If there is something currently on the workbench, put it in the cabinets.
 *   Look for the item outside and then in one of the cabinets. If it's nowhere then it must be new
 *   and add it to the workbench.
 */
    Result workOn(Item current_item) {
        if (!mWorkbench.empty()) {
            auto current_workbench_item = mWorkbench.pop_back();
            putInCabinet(current_workbench_item);
        }
        auto outside_item_it = mOutside.find(current_item);
        if (outside_item_it != mOutside.end()) {
            mOutside.pop(outside_item_it);
            mWorkbench.addItem(current_item);
            return Result("OUTSIDE");
        }
        for (int j = 0; j < mCabinets.size(); j++) {
            auto cabinet_item_it = mCabinets[j].find(current_item);
            if (cabinet_item_it != mCabinets[j].end()) {
                mCabinets[j].pop(cabinet_item_it);
                mWorkbench.addItem(current_item);
                return Result(j + 1);
            }
        }
        mWorkbench.addItem(current_item);
        return Result("NEW");
    }

private:
    Cabinet mOutside;
    Cabinet mWorkbench;
    std::vector<Cabinet> mCabinets;

/*
 *   \brief Put an Item in the cabinets.
 *   Push item in first cabinet. If it doesn't fit then the oldest item has to be pushed out
 *   and put in the next cabinet if available. Repeat until there is no more cabinets, and
 *   if we still have a remaining Item, then put it outside.
 */
    void putInCabinet(Item item) {
        Item extraItem(kSentinelItem);
        for (auto &cabinet: mCabinets) {
            extraItem = cabinet.addItem(item);
            if (extraItem != kSentinelItem) {
                item = extraItem;
            } else break;
        }
        if (item != kSentinelItem) {
            mOutside.addItem(extraItem);
        }
    }
};

/*
*   \brief Utility function to flag an invalid input.
*/
static void inputError() {
    std::cout << "INPUT_ERROR\n";
    exit(0);
}

/*
*   \brief Utility function to read an int64 from the stdin.
*/
inline int64_t readInt() {
    std::string input;
    getline(std::cin, input);
    try {
        return std::stoll(input);
    }
    catch (const std::invalid_argument &e) {
        inputError();
    }
    catch (const std::out_of_range &e) {
        inputError();
    }
    return kSentinelItem;
}

int main() {
// Read cabinet sizes
    std::string input;
    getline(std::cin, input);
    auto valid_read = std::all_of(input.begin(), input.end(), [](char c) {
        return (c == ' ' || std::isdigit(c));
    });
    if (!valid_read) {
        inputError();
    }

    auto is_invalid_cabinet_size = [](std::int64_t size) { return size <= 0 || size >= 1024; };
    std::istringstream iss(input);
    int cabinetSize;
    std::vector<int64_t> cabinetSizes;
    while (iss >> cabinetSize) {
        if (is_invalid_cabinet_size(cabinetSize)) {
            inputError();
        }
        cabinetSizes.push_back(cabinetSize);
    }
    if (cabinetSizes.size() > 64) {
        inputError();
    }

// Read nr of items to work on
    int64_t nrElements = readInt();
    if (nrElements == 0) {
        inputError();
    }

    WorkShop workshop(cabinetSizes);
// Read items and work on them
    Result res{};
    for (size_t i = 0; i < nrElements; i++) {
        res = workshop.workOn(readInt());
    }

    if (res.cabinet_nr != kSentinelItem) {
        std::cout << res.cabinet_nr << "\n";
    } else {
        std::cout << res.status << "\n";
    }
    return 0;
}

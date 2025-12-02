#pragma once
#include <iostream>
#include <memory>
#include <cstdint>

/// Before you ask, "Don't you need to keep track of the memory monitor??"
/// Well kind of, but not really
/// If you are curious, it's approximately (3 * unsigned int) + (1024 * MemoryMapIndex) = 12300 bytes/12.3 kilobytes
/// All of this lives in the stack, and it's not allocated dynamically
/// So it should be fineeee 
/// My biggest worry honestly is efficiency. Since it's just a plain array, as opposed to a map or vector
/// It doesn't do well when it comes searching
/// But parallel iterating might help...so 
/// TODO: Multithread it lmao
/// I ALSO REALIZED IT'S NOT MEMORY SAFE LMAO, DON'T USE IT UNLESS YOU KNOW WHAT YOU ARE DOING

struct MemoryMapEntry {
    unsigned int index;
    void* ptr;
    std::size_t size;
    MemoryMapEntry* childNode = nullptr;
};

class MEMORY_MAP {
public:
    static const unsigned int MAX_ENTRIES = 1024;
    unsigned int totalEntries; //Total amount of valid entries

    void AddEntry(void* memoryLocation, std::size_t numBytes) {
        totalEntries++;
        std::uintptr_t entryIndex = HashPtr(memoryLocation);

        //Creating the entry
        MemoryMapEntry newEntry{
            entryIndex, 
            memoryLocation, 
            numBytes
        };

        //Check if it's empty
        //If it is, then we just set that as the base entry
        if(entries[entryIndex].ptr == nullptr){
            entries[entryIndex] = newEntry;
        }
        //If not we traverse through and append it to the end
        else {
            MemoryMapEntry entryPivot = entries[entryIndex];
            while (entryPivot.childNode != nullptr) {
                entryPivot = *entryPivot.childNode;
            }

            entryPivot.childNode = &newEntry;
        }
    }



    void RemoveEntry(void* ptr) {
        //Searching through all the entries
        std::uintptr_t entryIndex = HashPtr(ptr);
        if (entries[entryIndex].ptr == ptr) {
            
            entries[entryIndex].ptr = nullptr;
            totalEntries--;
        }
        //If not we traverse through try and find it
        else {
            MemoryMapEntry* entryPivot = &entries[entryIndex];
            while (entryPivot->childNode != nullptr) {
                entryPivot = entryPivot->childNode;
                
                if (entryPivot->ptr == ptr) {
                    entryPivot->ptr = nullptr;
                    totalEntries--;
                    break;
                }
            }
        }

    }

    std::uintptr_t HashPtr(void* ptr) {
        return ((std::uintptr_t)ptr) % MAX_ENTRIES;
    }

    MemoryMapEntry* Get(void* memoryLocation) {
        std::uintptr_t index = HashPtr(memoryLocation);
        //Safeguards
        if(index < 0 || index >= MAX_ENTRIES){return nullptr;}
        
        //Check if it's the first element in the linked list
        if (entries[index].ptr == memoryLocation) {
            return &entries[index];
        }

        //If it's not
        //We traverse through the linked list until the end
        MemoryMapEntry* entryPivot = &entries[index];
        while (entryPivot->childNode != nullptr) {
            entryPivot = entryPivot->childNode;
            if (entryPivot->ptr == memoryLocation) {
                return &entries[index];
            }
        }

        return nullptr;
    }

    bool Find(void* memoryLocation) {
        if(Get(memoryLocation) != nullptr){return true;}
        return false;
    }

    static void Print(MEMORY_MAP &map) {
        int counter = 0;
        std::size_t total = 0;
        printf("======MEMORY MAP========================\n");
        for (int i = 0; i < MAX_ENTRIES; i++) {
            if (counter >= MAX_ENTRIES) {
                break;
            }

            //Found a valid entry
            if(map.entries[i].ptr != nullptr){
                counter++;
                total += map.entries[i].size;
                printf("%d  %p  %d bytes\n", map.entries[i].index, map.entries[i].ptr, map.entries[i].size);
            }
        }
        printf("========================================\n");
        printf("Entries: %d | Memory Used: %d bytes\n", map.totalEntries, total);
        printf("========================================\n\n");
    }
    
private:
    MemoryMapEntry entries[MAX_ENTRIES];
};

///
class MEMORY_CHUNKS{

public:
    static const unsigned int BYTES_PER_CHUNK = 20000000; //100 mil bytes 100 megabyte
    static const unsigned int CHUNKS = 4;
    static unsigned int USED[CHUNKS];

    //Add an entry to the memory chunks
    static void* AddEntry(std::size_t numBytes, int chunk) {
        //Return nullptr if it's not a valid chunk to allocate to
        if(chunk >= CHUNKS){return nullptr;}

        //If the amount of bytes you want to add doesn't fit, then it doesn't give you an address
        if (BYTES_PER_CHUNK - USED[chunk] < numBytes) {
            return nullptr;
        }
        

        void* ptr = std::malloc(numBytes);
        USED[chunk] += numBytes;
        MAPS[chunk].AddEntry(ptr, numBytes);
        return ptr;
    }

    //Automatically finds a chunk to store the entry
    static void* AutoAddEntry(std::size_t numBytes) {
        for (int i = 0; i < CHUNKS; i++) {
            if (BYTES_PER_CHUNK - USED[i] >= numBytes) {
                void* ptr = std::malloc(numBytes);
                USED[i] += numBytes;
                MAPS[i].AddEntry(ptr, numBytes);
                return ptr; 
            }
        }

        //If none of them are available return a nullptr
        return nullptr;
    }

    //Remove an entry from the memory chunk
    static void RemoveEntry(void* memoryLocation) {
        //Remove the chunk from the map
        for (int i = 0; i < CHUNKS; i++) {
            MemoryMapEntry* found = MAPS[i].Get(memoryLocation);
            if (found != nullptr) {
                USED[i] -= found->size;
                MAPS[i].RemoveEntry(memoryLocation);
            }
        }
        
        //Free it
        free(memoryLocation);
    }

    //Prints the memory map
    static void Print() {
        for (int i = 0; i < CHUNKS; i++) {
            float usage = (float)USED[i] / (float)BYTES_PER_CHUNK;
            printf("Chunk %d: %.5f%% (%d/%d)\n", i, usage, USED[i], BYTES_PER_CHUNK);
            MEMORY_MAP::Print(MAPS[i]);
            printf("\n");
        }
    }


private:
    static MEMORY_MAP MAPS[CHUNKS];
};

void* operator new(std::size_t numBytes) {
    void* ptr = MEMORY_CHUNKS::AutoAddEntry(numBytes);
    return ptr;
}

void* operator new(std::size_t numBytes, int chunk) {
    void* ptr = MEMORY_CHUNKS::AddEntry(numBytes, chunk);
    return ptr;
}

void operator delete(void* memoryLocation) {
    MEMORY_CHUNKS::RemoveEntry(memoryLocation);
}


//Initializing the static members
unsigned int MEMORY_CHUNKS::USED[CHUNKS];
MEMORY_MAP MEMORY_CHUNKS::MAPS[CHUNKS];

/// This is a trick to get access to numBytes ///
/// w/o wild typecasts. 
struct ArraySize {
    std::size_t numBytes;
};

/// I did a little hack to hide the total number of bytes
/// allocated in the array itself. 
void* operator new[](std::size_t numBytes, int chunk) {
    void* ptr = MEMORY_CHUNKS::AddEntry(numBytes + sizeof(ArraySize), chunk);
    ArraySize* array = reinterpret_cast<ArraySize*>(ptr);
    if (array) array->numBytes = numBytes;
    return array + 1;
}

/// This overload doesn't work as advertised in VS22
void operator delete[](void* memoryLocation, int chunk) {
    ArraySize* array = reinterpret_cast<ArraySize*>(memoryLocation) - 1;
    MEMORY_CHUNKS::RemoveEntry(memoryLocation);
}


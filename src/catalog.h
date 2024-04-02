#ifndef CATALOG_H
#define CATALOG_H

// @Todo: We are using poor man's id generation.
GLOBAL u32 catalog_item_id_counter;

// @Todo: Change items to index into 
template<typename T>
struct Catalog
{
    // Maps item ID to item data.
    Table<u32, T> table;
    
    // Maps item name to item ID.
    Table<String8, u32> names;
    
    // Stores item IDs contiguosly.
    Array<u32>  items;
};

template<typename T>
void catalog_init(Catalog<T> *catalog)
{
    table_init(&catalog->table);
    table_init(&catalog->names);
    array_init(&catalog->items);
}

template<typename T>
void catalog_free(Catalog<T> *catalog)
{
    table_free(&catalog->table);
    table_free(&catalog->names);
    array_free(&catalog->items);
}

template<typename T>
void add(Catalog<T> *catalog, String8 name, T item)
{
    u32 new_item_id = catalog_item_id_counter++;
    table_add(&catalog->table, new_item_id, item);
    table_add(&catalog->names, name, new_item_id);
    array_add(&catalog->items, new_item_id);
}

template<typename T>
T* find(Catalog<T> *catalog, String8 name)
{
    b32 found;
    u32 item_id = table_find(&catalog->names, name, &found);
    if (found)
    {
        T* item = table_find_pointer(&catalog->table, item_id);
        return item;
    }
    
    return 0;
}

template<typename T>
T* find_from_index(Catalog<T> *catalog, u32 item_index)
{
    // The provided index is relative to our items array.
    ASSERT(item_index >= 0 && item_index < catalog->items.count);
    
    u32 item_id = catalog->items[item_index];
    T* item = table_find_pointer(&catalog->table, item_id);
    return item;
}

#endif //CATALOG_H

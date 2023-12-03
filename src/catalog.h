#ifndef CATALOG_H
#define CATALOG_H

template<typename K, typename V>
struct Catalog
{
    Table<K, V> table;
    Array<V*>   items;
};

template<typename K, typename V>
void catalog_init(Catalog<K, V> *catalog)
{
    table_init(&catalog->table);
    array_init(&catalog->items);
}

template<typename K, typename V>
V* add(Catalog<K, V> *catalog, K key, V item)
{
    V* new_item = table_add(&catalog->table, key, item);
    array_add(&catalog->items, new_item);
    
    return new_item;
}

template<typename K, typename V>
V* find(Catalog<K, V> *catalog, K key)
{
    V* item = table_find_pointer(&catalog->table, key);
    return item;
}


#endif //CATALOG_H

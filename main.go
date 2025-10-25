package main

import (
  "container/list"
  "fmt"
  "html/template"
  "net/http"
  "strconv"
  "sync"
  "time"
)

// ********************** CACHE LOGIC ************************************

type entry struct{
key string
value string
}
var entrypool = sync.Pool{
  New: func() any {return  new(entry},
}

type LRUCACHE struct{
capacity int
  ll *list.List
  cache *map[string]*list.Element
  mu sync.RWMutex

  hits, misses,evictions int
}
func NewLRU(cap *int) *LRUCache {
return LRUCache{

  
}
  
}                

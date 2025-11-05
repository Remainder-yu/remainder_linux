#include <stdio.h>
#include <stdint.h>

typedef uint64_t pteval_t;
typedef struct {
    pteval_t pte;
} pte_t;

// 宏定义：获取页表项的原始值
#define pte_val(x) ((x).pte)
// 将原始值包装为pte_t类型
#define __pte(x) ((pte_t) { (x) })

// 页表项标志位（复用之前的ARM PTE定义）
#define PTE_TYPE_MASK  (3UL << 0)
#define PTE_TYPE_PAGE  (3UL << 0)    // 页映射类型
#define PTE_USER       (1UL << 6)    // 允许用户模式访问
#define PTE_RDONLY     (1UL << 7)    // 只读
#define PTE_UXN        (1UL << 54)   // 用户模式不可执行

pte_t create_user_rw_exec_pte(pteval_t phyaddr) {
    pteval_t val = 0;
    val |= (phyaddr & ~0xFFFULL);
    val |= PTE_TYPE_PAGE;
    val |= PTE_USER;
    return __pte(val);
}

int main() {
    pteval_t phyaddr = 0x80000000;
    pte_t my_pte = create_user_rw_exec_pte(phyaddr);
    printf(" create PTE value: 0x%lx\n", pte_val(my_pte));

    return 0;
}

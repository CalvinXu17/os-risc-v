int main(void)
{
    while(1)
    {
        asm volatile("wfi");
    }
    return 0;
}
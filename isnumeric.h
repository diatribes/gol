#ifndef ISNUMERIC_H
#define ISNUMERIC_H
static int isnumeric(char *str)
{
    while(*str)
    {
        if(!isdigit(*str))
        {
            return 0;
        }
        str++;
    }
    return 1;
}
#endif

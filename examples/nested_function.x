int n = 10

def outer(x)
    int n = 20
    float pi = 3.14
    
    def inner(x)
        int n = 50
        print n, pi
        print x * n
        
    inner(x)

outer(5)

#if !defined(__ERROR_HPP)
#define      __ERROR_HPP

#include <iostream>
#include <sstream>

enum Severity {
    FATAL,
    WARNING,
    INFO,
};
class Error{
public:
    Error() : sev(FATAL)
    {
        
    }

    template <class T>
    void operator<<(T data){
        std::cout << "FATAL ERROR: " << data << std::endl;
        exit( -1 );
    }

private:
    std::stringstream stream;
    Severity sev;
};

#endif /* __ERROR_HPP */
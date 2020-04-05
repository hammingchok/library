#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

class noncopyable {
protected:
    noncopyable(){};
    ~noncopyable(){};

private:
    //将拷贝构造函数和重载=设置为private
    noncopyable(const noncopyable&);
    const noncopyable& operator=(const noncopyable&);
};

#endif

//
// Created by oleg on 23.04.24.
//

#ifndef STREAMINGGATEWAYCROW_BASETEST_H
#define STREAMINGGATEWAYCROW_BASETEST_H


class BaseTest {
public:
    virtual void runTests() = 0;
protected:
    virtual void setUp() = 0;
    virtual void tearDown() = 0;
};


#endif //STREAMINGGATEWAYCROW_BASETEST_H

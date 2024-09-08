#pragma once

class FlagManager
{
private:
  bool in_flag_     = false;  //含义是左表该不该前进
  bool not_in_flag_ = false;  //含义是右表有没有到最后一个
  // Private constructor to prevent instantiation
  FlagManager() {}

public:
  // Singleton pattern to get the unique instance
  static FlagManager &getInstance()
  {
    static FlagManager instance;
    return instance;
  }

  // Prevent copying and assignment
  FlagManager(const FlagManager &)    = delete;
  void operator=(const FlagManager &) = delete;

  // Getter and setter for flag
  void setLeftMove(bool value) { in_flag_ = value; }
  bool getLeftMove() const { return in_flag_; }

  void setRightEnd(bool value) { not_in_flag_ = value; }
  bool getRightEnd() const { return not_in_flag_; }
};